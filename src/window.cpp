#include "window.h"

#include <giomm/file.h>
#include <giomm/liststore.h>
#include <giomm/menuitem.h>
#include <glibmm/variant.h>
#include <glibmm/convert.h>
#include <glibmm/value.h>
#include <glibmm/wrap.h>
#include <gtkmm/builder.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/widget.h>

#include <glibmm/i18n.h>

#include "core/i18n.h"
#include "modules/file.h"
#include "modules/theme.h"

namespace Notepad {

namespace {

// Same C-callback trampoline pattern as commands.cpp/dialog.cpp --
// AdwAlertDialog has no gtkmm binding.
void background_opacity_choose_trampoline(GObject *source, GAsyncResult *result,
                                           gpointer user_data) {
  auto *callback = static_cast<std::function<void(const Glib::ustring &)> *>(user_data);
  const char *response = adw_alert_dialog_choose_finish(ADW_ALERT_DIALOG(source), result);
  (*callback)(response ? response : "");
  delete callback;
}

int next_zoom_in(int current) {
  for (int level : kZoomSteps) {
    if (level > current) {
      return level;
    }
  }
  return current;
}

int next_zoom_out(int current) {
  for (auto it = kZoomSteps.rbegin(); it != kZoomSteps.rend(); ++it) {
    if (*it < current) {
      return *it;
    }
  }
  return current;
}

} // namespace

Window::Window(GtkApplication *app, AppState &state, const Settings &settings)
    : state_(state), settings_(settings), editor_(editor_view_),
      commands_(*this, editor_, state_, settings_),
      dialogs_(*this, editor_, state_, settings_) {
  window_ = GTK_WIDGET(adw_application_window_new(app));

  auto *toolbar_view = adw_toolbar_view_new();
  auto *header_bar = adw_header_bar_new();

  build_menu();
  adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar),
                             GTK_WIDGET(menu_button_.gobj()));
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header_bar);
  // Phase EXTRA A: classic menu row, directly under the header bar.
  classic_menu_bar_.set_halign(Gtk::Align::START);
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view),
                                GTK_WIDGET(classic_menu_bar_.gobj()));
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view),
                                GTK_WIDGET(dialogs_.find_bar().gobj()));

  scroller_.set_child(editor_view_);
  scroller_.set_vexpand(true);
  scroller_.set_hexpand(true);
  // Gtk::Overlay only stretches an add_overlay() child to fill the overlay
  // area if its halign/valign are FILL *in addition to* hexpand/vexpand --
  // hexpand/vexpand alone (set above) still leaves it sized to its natural
  // request and centered, which is exactly the tiny floating square bug.
  scroller_.set_halign(Gtk::Align::FILL);
  scroller_.set_valign(Gtk::Align::FILL);
  editor_view_.set_monospace(true);
  editor_.apply_word_wrap(state_.word_wrap);
  editor_.apply_font(state_);

  // Background image (Phase 9) renders into background_area_, sitting
  // behind scroller_ in an overlay; editor_.set_transparent_background()
  // lets it show through the text view when a background is enabled.
  background_area_.set_draw_func([this](const Cairo::RefPtr<Cairo::Context> &cr,
                                         int width, int height) {
    if (state_.background.enabled) {
      background_.draw(cr, width, height, state_.background.position,
                        state_.background.opacity);
    }
  });
  overlay_.set_child(background_area_);
  overlay_.add_overlay(scroller_);
  overlay_.set_vexpand(true);
  overlay_.set_hexpand(true);

  if (state_.background.enabled && !state_.background.image_path.empty()) {
    Glib::ustring error;
    if (background_.load_image(state_.background.image_path, error)) {
      editor_.set_transparent_background(true);
    } else {
      state_.background.enabled = false;
    }
  }

  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view),
                                GTK_WIDGET(overlay_.gobj()));

  build_status_row();
  adw_toolbar_view_add_bottom_bar(ADW_TOOLBAR_VIEW(toolbar_view),
                                   GTK_WIDGET(status_box_.gobj()));

  adw_application_window_set_content(ADW_APPLICATION_WINDOW(window_),
                                      toolbar_view);

  gtk_window_set_default_size(GTK_WINDOW(window_), state_.window_width,
                               state_.window_height);

  // AdwApplicationWindow derives from GtkApplicationWindow, which has a real
  // gtkmm wrapper implementing Gio::ActionMap (needed for add_action() to
  // register "win.*" actions resolvable by the GMenu built in build_menu()).
  window_wrap_ = Glib::wrap(GTK_APPLICATION_WINDOW(window_));

  if (state_.window_maximized) {
    window_wrap_->maximize();
  }

  register_actions();
  commands_.register_actions();
  dialogs_.register_actions();
  setup_drop_target();

  auto buffer = editor_view_.get_buffer();
  buffer->signal_changed().connect([this]() {
    state_.modified = true;
    update_title();
    update_status();
  });
  buffer->signal_mark_set().connect(
      [this](const Gtk::TextBuffer::iterator &,
             const Glib::RefPtr<Gtk::TextBuffer::Mark> &) { update_status(); });

  status_box_.set_visible(state_.show_statusbar);
  update_title();
  update_status();
  rebuild_recent_files_menu();
}

Window::~Window() = default;

void Window::present() { window_wrap_->present(); }

void Window::refresh() {
  update_title();
  update_status();
  rebuild_recent_files_menu();
}

void Window::open_external_path(const Glib::ustring &path) {
  commands_.open_path(path);
  refresh();
}

void Window::save_state(AppState &state) const {
  state.window_maximized = window_wrap_->is_maximized();
  if (!state.window_maximized) {
    state.window_width = window_wrap_->get_width();
    state.window_height = window_wrap_->get_height();
  }
}

// Re-translates everything that was only ever resolved once, at
// construction time -- language_action_'s handler calls this right after
// apply_language_override() (core/i18n.h) rewrites LANGUAGE, so the switch
// is visible immediately with no relaunch. build_menu() re-parses menus.ui
// from scratch, which re-runs gettext for every translatable="yes" string
// and the hardcoded classic-bar labels, and swaps the new GMenuModels into
// the already-existing menu_button_/classic_menu_bar_ widgets.
// dialogs_.refresh_translations() covers the Find/Replace bar's labels,
// the only other construct-once translated strings in the app; everything
// else (dialogs, file choosers, title, status bar) already calls _() fresh
// each time it's shown, via refresh().
void Window::refresh_translations() {
  build_menu();
  dialogs_.refresh_translations();
  refresh();
}

void Window::build_menu() {
  auto builder = Gtk::Builder::create_from_resource(
      "/io/github/chiddekel/Notepad/ui/menus.ui");
  auto menu_model =
      std::dynamic_pointer_cast<Gio::MenuModel>(builder->get_object("menubar"));
  menu_button_.set_menu_model(menu_model);
  menu_button_.set_icon_name("open-menu-symbolic");

  // Retrieved by id so rebuild_recent_files_menu() can repopulate it at
  // runtime -- see the comment in menus.ui above <submenu id="recent-files-menu">.
  recent_files_menu_ =
      std::dynamic_pointer_cast<Gio::Menu>(builder->get_object("recent-files-menu"));

  // Phase EXTRA A: classic File/Edit/Format/View/Help menu bar, reusing the
  // same submenu GMenuModels (not copies) as menu_button_ above -- items,
  // actions, and state all stay in sync between the two presentations
  // automatically since they're the same underlying GMenu objects.
  // Language is deliberately left out of this bar -- it stays hamburger-menu
  // only, since menu_button_ already gets it via the full "menubar" model.
  auto classic_bar_model = Gio::Menu::create();
  struct { const char *id; const char *label; } classic_menus[] = {
      {"file-menu", "_File"},
      {"edit-menu", "_Edit"},
      {"format-menu", "F_ormat"},
      {"view-menu", "_View"},
      {"help-menu", "_Help"},
  };
  for (const auto &entry : classic_menus) {
    auto submenu = std::dynamic_pointer_cast<Gio::MenuModel>(builder->get_object(entry.id));
    if (submenu) {
      classic_bar_model->append_submenu(_(entry.label), submenu);
    }
  }
  classic_menu_bar_.set_menu_model(classic_bar_model);
}

void Window::build_status_row() {
  status_box_.set_margin_start(12);
  status_box_.set_margin_end(12);
  status_box_.set_margin_top(4);
  status_box_.set_margin_bottom(4);
  status_box_.set_halign(Gtk::Align::END);
  status_box_.append(status_position_);
  status_box_.append(status_encoding_);
  status_box_.append(status_line_ending_);
  status_box_.append(status_zoom_);
}

void Window::register_actions() {
  word_wrap_action_ = window_wrap_->add_action_bool(
      "word-wrap",
      [this]() {
        bool current = false;
        word_wrap_action_->get_state(current);
        bool next = !current;
        word_wrap_action_->change_state(next);
        state_.word_wrap = next;
        editor_.apply_word_wrap(next);
        settings_.save_editor_prefs(state_);
      },
      state_.word_wrap);

  status_bar_action_ = window_wrap_->add_action_bool(
      "status-bar",
      [this]() {
        bool current = false;
        status_bar_action_->get_state(current);
        bool next = !current;
        status_bar_action_->change_state(next);
        state_.show_statusbar = next;
        status_box_.set_visible(next);
        settings_.save_editor_prefs(state_);
      },
      state_.show_statusbar);

  // Best-effort only: GTK4 has no public API for always-on-top (Wayland
  // compositors generally refuse it, and even the X11-era GTK3 hint was
  // removed). This wires the toggle/state/persistence; it intentionally has
  // no windowing effect on this platform, matching the locked plan decision.
  always_on_top_action_ = window_wrap_->add_action_bool(
      "always-on-top",
      [this]() {
        bool current = false;
        always_on_top_action_->get_state(current);
        bool next = !current;
        always_on_top_action_->change_state(next);
        state_.always_on_top = next;
        settings_.save_font(state_);
      },
      state_.always_on_top);

  // Legacy has a single on/off Dark Mode checkbox (not a System/Light/Dark
  // radio group), so this toggles between ColorScheme::Dark and
  // ColorScheme::System rather than exposing all three states.
  dark_mode_action_ = window_wrap_->add_action_bool(
      "dark-mode",
      [this]() {
        bool current = false;
        dark_mode_action_->get_state(current);
        bool next = !current;
        dark_mode_action_->change_state(next);
        state_.color_scheme = next ? ColorScheme::Dark : ColorScheme::System;
        Theme::apply_color_scheme(state_.color_scheme);
        settings_.save_color_scheme(state_.color_scheme);
      },
      state_.color_scheme == ColorScheme::Dark);

  // Unlike legacy's ModifyMenuW-based menu rewrite, this doesn't rebuild
  // native Win32 menu resources -- but GNU gettext re-reads LANGUAGE on
  // every _() call rather than caching it once at startup, so switching
  // languages live in-process (no relaunch) just means: rewrite LANGUAGE
  // and re-derive LC_MESSAGES from it (apply_language_override(),
  // core/i18n.h), flip menu/toolbar mirroring for right-to-left languages,
  // and re-run everything that only called _() once, at construction time
  // (refresh_translations()).
  language_action_ = window_wrap_->add_action_radio_string(
      "language",
      [this](const Glib::ustring &nick) {
        language_action_->change_state(nick);
        state_.language_override = nick;
        settings_.save_language_override(nick);

        apply_language_override(nick);
        Gtk::Widget::set_default_direction(
            is_rtl_language(nick) ? Gtk::TextDirection::RTL : Gtk::TextDirection::LTR);
        refresh_translations();
      },
      state_.language_override.empty() ? "en" : state_.language_override);

  // Recent Files submenu (legacy's UpdateRecentFilesMenu()). has-recent-files
  // is never activated by the user -- only its enabled state matters, since
  // menus.ui's <submenu hidden-when="action-disabled"> hides the whole
  // submenu when the list is empty.
  has_recent_files_action_ = window_wrap_->add_action("has-recent-files", []() {});
  has_recent_files_action_->set_enabled(!state_.recent_files.empty());

  window_wrap_->add_action_with_parameter(
      "open-recent", Glib::VariantType("s"),
      [this](const Glib::VariantBase &parameter) {
        auto path = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(parameter).get();
        commands_.open_path(path);
        refresh();
      });

  window_wrap_->add_action("zoom-in", [this]() {
    state_.zoom_level = next_zoom_in(state_.zoom_level);
    editor_.apply_zoom(state_);
    settings_.save_editor_prefs(state_);
    update_status();
  });
  window_wrap_->add_action("zoom-out", [this]() {
    state_.zoom_level = next_zoom_out(state_.zoom_level);
    editor_.apply_zoom(state_);
    settings_.save_editor_prefs(state_);
    update_status();
  });
  window_wrap_->add_action("zoom-default", [this]() {
    state_.zoom_level = kZoomDefault;
    editor_.apply_zoom(state_);
    settings_.save_editor_prefs(state_);
    update_status();
  });

  window_wrap_->add_action("background-select", [this]() {
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title(_("Select Background Image"));

    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    auto image_filter = Gtk::FileFilter::create();
    image_filter->set_name(_("Image Files"));
    for (const char *pattern : {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"}) {
      image_filter->add_pattern(pattern);
    }
    filters->append(image_filter);
    dialog->set_filters(filters);

    dialog->open(*window_wrap_,
                 [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
                   try {
                     auto file = dialog->open_finish(result);
                     if (!file) {
                       return;
                     }
                     Glib::ustring path = file->get_path();
                     Glib::ustring error;
                     if (background_.load_image(path, error)) {
                       state_.background.image_path = path;
                       state_.background.enabled = true;
                       editor_.set_transparent_background(true);
                       settings_.save_background(state_.background);
                       background_area_.queue_draw();
                     }
                   } catch (const Glib::Error &) {
                     // Cancelled or failed; nothing to do.
                   }
                 });
  });

  window_wrap_->add_action("background-clear", [this]() {
    background_.clear_image();
    state_.background.enabled = false;
    state_.background.image_path.clear();
    editor_.set_transparent_background(false);
    settings_.save_background(state_.background);
    background_area_.queue_draw();
  });

  window_wrap_->add_action("background-opacity", [this]() {
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), state_.background.opacity * 100);
    gtk_widget_set_size_request(scale, 240, -1);

    AdwDialog *dialog = adw_alert_dialog_new(_("Background Opacity"), nullptr);
    auto *alert = ADW_ALERT_DIALOG(dialog);
    adw_alert_dialog_set_extra_child(alert, scale);
    adw_alert_dialog_add_responses(alert, "cancel", _("_Cancel"), "apply", _("_Apply"), nullptr);
    adw_alert_dialog_set_response_appearance(alert, "apply", ADW_RESPONSE_SUGGESTED);
    adw_alert_dialog_set_default_response(alert, "apply");
    adw_alert_dialog_set_close_response(alert, "cancel");

    auto *callback = new std::function<void(const Glib::ustring &)>(
        [this, scale](const Glib::ustring &response) {
          if (response == "apply") {
            state_.background.opacity = gtk_range_get_value(GTK_RANGE(scale)) / 100.0;
            settings_.save_background(state_.background);
            background_area_.queue_draw();
          }
        });
    adw_alert_dialog_choose(alert, GTK_WIDGET(window_wrap_->gobj()), nullptr,
                             background_opacity_choose_trampoline, callback);
  });

  background_position_action_ = window_wrap_->add_action_radio_string(
      "background-position",
      [this](const Glib::ustring &nick) {
        background_position_action_->change_state(nick);
        state_.background.position = bg_position_from_nick(nick);
        settings_.save_background(state_.background);
        background_area_.queue_draw();
      },
      to_nick(state_.background.position));
}

void Window::setup_drop_target() {
  auto drop_target =
      Gtk::DropTarget::create(Gio::File::get_type(), Gdk::DragAction::COPY);
  drop_target->signal_drop().connect(
      [this](const Glib::ValueBase &value, double, double) -> bool {
        Glib::Value<Glib::RefPtr<Gio::File>> file_value;
        file_value.init(value.gobj());
        auto file = file_value.get();
        if (!file) {
          return false;
        }
        commands_.open_path(file->get_path());
        return true;
      },
      false);
  window_wrap_->add_controller(drop_target);
}

void Window::update_title() {
  Glib::ustring name = state_.file_path.empty()
                            ? Glib::ustring(_("Untitled"))
                            : Glib::filename_display_basename(state_.file_path.raw());
  Glib::ustring title = (state_.modified ? "*" : "") + name + " — Notepad";
  gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
}

void Window::update_status() {
  auto [line, col] = editor_.get_cursor_pos();
  // Translators: cursor position in the status bar, e.g. "Ln 3, Col 12".
  status_position_.set_text(
      Glib::ustring::compose(_("Ln %1, Col %2"), line, col));
  status_encoding_.set_text(encoding_display_name(state_.encoding));
  status_line_ending_.set_text(line_ending_display_name(state_.line_ending));
  status_zoom_.set_text(Glib::ustring::compose("%1%%", state_.zoom_level));
}

void Window::rebuild_recent_files_menu() {
  if (has_recent_files_action_) {
    has_recent_files_action_->set_enabled(!state_.recent_files.empty());
  }
  if (!recent_files_menu_) {
    return;
  }
  recent_files_menu_->remove_all();
  for (const auto &path : state_.recent_files) {
    auto item = Gio::MenuItem::create(Glib::filename_display_basename(path.raw()),
                                       Glib::ustring());
    item->set_action_and_target("win.open-recent", Glib::Variant<Glib::ustring>::create(path));
    recent_files_menu_->append_item(item);
  }
}

} // namespace Notepad
