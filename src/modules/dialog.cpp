#include "dialog.h"

#include <adwaita.h>
#include <algorithm>
#include <functional>
#include <gtkmm/fontdialog.h>
#include <gtkmm/textbuffer.h>

#include <glibmm/i18n.h>

#include "config.h"
#include "window.h"

namespace Notepad {

namespace {

// Same C-callback trampoline pattern as commands.cpp's ConfirmDiscard --
// AdwAlertDialog has no gtkmm binding (see the plan's Architecture
// correction note), so its async choose() needs a plain-C bridge into a
// heap-allocated std::function.
void alert_dialog_choose_trampoline(GObject *source, GAsyncResult *result,
                                     gpointer user_data) {
  auto *callback = static_cast<std::function<void(const Glib::ustring &)> *>(user_data);
  const char *response = adw_alert_dialog_choose_finish(ADW_ALERT_DIALOG(source), result);
  (*callback)(response ? response : "");
  delete callback;
}

} // namespace

Dialogs::Dialogs(Window &window, Editor &editor, AppState &state,
                  const Settings &settings)
    : window_(window), editor_(editor), state_(state), settings_(settings) {
  search_entry_.set_hexpand(true);
  replace_entry_.set_hexpand(true);
  replace_entry_.set_placeholder_text(_("Replace with…"));
  replace_button_.set_label(_("Replace"));
  replace_all_button_.set_label(_("Replace All"));

  find_row_.append(search_entry_);
  find_row_.append(find_prev_button_);
  find_row_.append(find_next_button_);
  find_row_.append(close_button_);

  replace_row_.append(replace_entry_);
  replace_row_.append(replace_button_);
  replace_row_.append(replace_all_button_);

  find_box_.set_margin_start(12);
  find_box_.set_margin_end(12);
  find_box_.set_margin_top(6);
  find_box_.set_margin_bottom(6);
  find_box_.append(find_row_);
  find_box_.append(replace_row_);
  replace_row_.set_visible(false);

  revealer_.set_child(find_box_);
  revealer_.set_reveal_child(false);
  revealer_.set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);

  search_entry_.signal_activate().connect(sigc::mem_fun(*this, &Dialogs::find_next));
  search_entry_.signal_search_changed().connect(sigc::mem_fun(*this, &Dialogs::find_next));
  find_next_button_.signal_clicked().connect(sigc::mem_fun(*this, &Dialogs::find_next));
  find_prev_button_.signal_clicked().connect(sigc::mem_fun(*this, &Dialogs::find_previous));
  close_button_.signal_clicked().connect(sigc::mem_fun(*this, &Dialogs::hide_find_bar));
  replace_button_.signal_clicked().connect(sigc::mem_fun(*this, &Dialogs::do_replace));
  replace_all_button_.signal_clicked().connect(sigc::mem_fun(*this, &Dialogs::do_replace_all));
}

void Dialogs::refresh_translations() {
  replace_button_.set_label(_("Replace"));
  replace_all_button_.set_label(_("Replace All"));
  replace_entry_.set_placeholder_text(_("Replace with…"));
}

void Dialogs::register_actions() {
  auto *win = window_.gtk_window();
  win->add_action("find", [this]() { show_find_bar(false); });
  win->add_action("replace", [this]() { show_find_bar(true); });
  win->add_action("find-next", sigc::mem_fun(*this, &Dialogs::find_next));
  win->add_action("find-previous", sigc::mem_fun(*this, &Dialogs::find_previous));
  win->add_action("goto", sigc::mem_fun(*this, &Dialogs::show_goto_dialog));
  win->add_action("font", sigc::mem_fun(*this, &Dialogs::show_font_dialog));
  win->add_action("about", sigc::mem_fun(*this, &Dialogs::show_about_dialog));
  win->add_action("transparency", sigc::mem_fun(*this, &Dialogs::show_transparency_dialog));
}

void Dialogs::show_find_bar(bool with_replace) {
  replace_row_.set_visible(with_replace);
  revealer_.set_reveal_child(true);
  search_entry_.grab_focus();
  if (!state_.find_text.empty()) {
    search_entry_.set_text(state_.find_text);
    search_entry_.select_region(0, -1);
  }
}

void Dialogs::hide_find_bar() { revealer_.set_reveal_child(false); }

void Dialogs::find_next() {
  Glib::ustring needle = search_entry_.get_text();
  if (needle.empty()) {
    return;
  }
  state_.find_text = needle;
  auto buffer = editor_.view().get_buffer();
  auto start = buffer->get_iter_at_mark(buffer->get_selection_bound());

  Gtk::TextBuffer::iterator match_start, match_end;
  bool found = start.forward_search(needle, Gtk::TextSearchFlags::CASE_INSENSITIVE,
                                     match_start, match_end);
  if (!found) {
    // Wrap around from the top of the document.
    found = buffer->begin().forward_search(needle, Gtk::TextSearchFlags::CASE_INSENSITIVE,
                                            match_start, match_end);
  }
  if (found) {
    buffer->select_range(match_start, match_end);
    editor_.view().scroll_to(match_start, 0.1);
  }
}

void Dialogs::find_previous() {
  Glib::ustring needle = search_entry_.get_text();
  if (needle.empty()) {
    return;
  }
  state_.find_text = needle;
  auto buffer = editor_.view().get_buffer();
  auto start = buffer->get_iter_at_mark(buffer->get_insert());

  Gtk::TextBuffer::iterator match_start, match_end;
  bool found = start.backward_search(needle, Gtk::TextSearchFlags::CASE_INSENSITIVE,
                                      match_start, match_end);
  if (!found) {
    // Wrap around from the end of the document.
    found = buffer->end().backward_search(needle, Gtk::TextSearchFlags::CASE_INSENSITIVE,
                                           match_start, match_end);
  }
  if (found) {
    buffer->select_range(match_end, match_start);
    editor_.view().scroll_to(match_start, 0.1);
  }
}

void Dialogs::do_replace() {
  auto buffer = editor_.view().get_buffer();
  Gtk::TextBuffer::iterator sel_start, sel_end;
  if (buffer->get_selection_bounds(sel_start, sel_end)) {
    Glib::ustring selected = buffer->get_text(sel_start, sel_end);
    if (selected.casefold() == search_entry_.get_text().casefold()) {
      buffer->erase(sel_start, sel_end);
      buffer->insert_at_cursor(replace_entry_.get_text());
    }
  }
  find_next();
}

void Dialogs::do_replace_all() {
  Glib::ustring needle = search_entry_.get_text();
  if (needle.empty()) {
    return;
  }
  Glib::ustring replacement = replace_entry_.get_text();
  auto buffer = editor_.view().get_buffer();

  int search_from = 0;
  int replaced = 0;
  while (replaced < 1'000'000) { // safety guard, not a realistic document size
    auto iter = buffer->get_iter_at_offset(search_from);
    Gtk::TextBuffer::iterator match_start, match_end;
    if (!iter.forward_search(needle, Gtk::TextSearchFlags::CASE_INSENSITIVE,
                              match_start, match_end)) {
      break;
    }
    int offset = match_start.get_offset();
    buffer->erase(match_start, match_end);
    auto insert_pos = buffer->get_iter_at_offset(offset);
    buffer->insert(insert_pos, replacement);
    search_from = offset + static_cast<int>(replacement.size());
    ++replaced;
  }
}

void Dialogs::show_goto_dialog() {
  auto buffer = editor_.view().get_buffer();
  int line_count = std::max(1, buffer->get_line_count());

  GtkWidget *spin = gtk_spin_button_new_with_range(1, line_count, 1);
  auto current_line = buffer->get_iter_at_mark(buffer->get_insert()).get_line();
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), current_line + 1);

  AdwDialog *dialog = adw_alert_dialog_new(_("Go to Line"), nullptr);
  auto *alert = ADW_ALERT_DIALOG(dialog);
  adw_alert_dialog_set_extra_child(alert, spin);
  adw_alert_dialog_add_responses(alert, "cancel", _("_Cancel"), "go", _("_Go To"), nullptr);
  adw_alert_dialog_set_response_appearance(alert, "go", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(alert, "go");
  adw_alert_dialog_set_close_response(alert, "cancel");

  auto *callback = new std::function<void(const Glib::ustring &)>(
      [this, spin](const Glib::ustring &response) {
        if (response == "go") {
          int line = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin))) - 1;
          auto buffer = editor_.view().get_buffer();
          auto iter = buffer->get_iter_at_line(line);
          buffer->place_cursor(iter);
          editor_.view().scroll_to(iter, 0.1);
        }
      });
  adw_alert_dialog_choose(alert, GTK_WIDGET(window_.gtk_window()->gobj()), nullptr,
                           alert_dialog_choose_trampoline, callback);
}

void Dialogs::show_font_dialog() {
  auto dialog = Gtk::FontDialog::create();
  dialog->set_title(_("Choose Font"));

  Pango::FontDescription initial;
  initial.set_family(state_.font_name);
  initial.set_size(state_.font_size * PANGO_SCALE);
  initial.set_weight(static_cast<Pango::Weight>(state_.font_weight));
  initial.set_style(state_.font_italic ? Pango::Style::ITALIC : Pango::Style::NORMAL);

  dialog->choose_font(
      *window_.gtk_window(),
      [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
          Pango::FontDescription chosen = dialog->choose_font_finish(result);
          state_.font_name = chosen.get_family();
          state_.font_size = std::max(kMinFontSize,
                                       std::min(kMaxFontSize, chosen.get_size() / PANGO_SCALE));
          state_.font_weight = static_cast<int>(chosen.get_weight());
          state_.font_italic = chosen.get_style() != Pango::Style::NORMAL;
          editor_.apply_font(state_);
          settings_.save_font(state_);
        } catch (const Glib::Error &) {
          // Cancelled or failed; nothing to do.
        }
      },
      initial);
}

void Dialogs::show_about_dialog() {
  AdwDialog *dialog = adw_about_dialog_new();
  auto *about = ADW_ABOUT_DIALOG(dialog);
  adw_about_dialog_set_application_name(about, "Notepad");
  adw_about_dialog_set_application_icon(about, APP_ID);
  adw_about_dialog_set_version(about, VERSION);
  adw_about_dialog_set_developer_name(about, "chiddekel");
  adw_about_dialog_set_website(about, "https://chiddekel.github.io/gnome-notepad/");
  adw_about_dialog_set_issue_url(about, "https://github.com/chiddekel/gnome-notepad/issues");
  adw_about_dialog_set_copyright(about, "© 2026 chiddekel");
  adw_about_dialog_set_license_type(about, GTK_LICENSE_MIT_X11);

  // Credits.
  const char *developers[] = {"chiddekel <qp2boih4e@mozmail.com>", nullptr};
  adw_about_dialog_set_developers(about, developers);

  // GNOME convention: each po/<locale>.po supplies its own translation of
  // the literal msgid "translator-credits" (name/email per line). gettext
  // returns the msgid itself when there's no translation for the current
  // locale, so that has to be treated as "no credits" rather than shown
  // literally.
  Glib::ustring translator_credits = _("translator-credits");
  if (translator_credits != "translator-credits") {
    adw_about_dialog_set_translator_credits(about, translator_credits.c_str());
  }

  const char *concept_credit[] = {"ForLoopCodes (original legacy-notepad concept)", nullptr};
  adw_about_dialog_add_credit_section(about, _("Based on"), concept_credit);

  adw_dialog_present(dialog, GTK_WIDGET(window_.gtk_window()->gobj()));
}

void Dialogs::show_transparency_dialog() {
  // Best-effort only: window opacity may not be composited correctly on
  // every Wayland compositor (same caveat as always-on-top). Wired via
  // plain gtk_widget_set_opacity(), the only mechanism GTK4 offers here.
  GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 20, 100, 1);
  gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
  gtk_range_set_value(
      GTK_RANGE(scale),
      gtk_widget_get_opacity(GTK_WIDGET(window_.gtk_window()->gobj())) * 100);
  gtk_widget_set_size_request(scale, 240, -1);

  AdwDialog *dialog = adw_alert_dialog_new(_("Window Transparency"), nullptr);
  auto *alert = ADW_ALERT_DIALOG(dialog);
  adw_alert_dialog_set_extra_child(alert, scale);
  adw_alert_dialog_add_responses(alert, "cancel", _("_Cancel"), "apply", _("_Apply"), nullptr);
  adw_alert_dialog_set_response_appearance(alert, "apply", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(alert, "apply");
  adw_alert_dialog_set_close_response(alert, "cancel");

  auto *callback = new std::function<void(const Glib::ustring &)>(
      [this, scale](const Glib::ustring &response) {
        if (response == "apply") {
          double value = gtk_range_get_value(GTK_RANGE(scale)) / 100.0;
          gtk_widget_set_opacity(GTK_WIDGET(window_.gtk_window()->gobj()), value);
        }
      });
  adw_alert_dialog_choose(alert, GTK_WIDGET(window_.gtk_window()->gobj()), nullptr,
                           alert_dialog_choose_trampoline, callback);
}

} // namespace Notepad
