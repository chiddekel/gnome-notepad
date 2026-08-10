#include "commands.h"

#include <adwaita.h>
#include <cmath>
#include <giomm/liststore.h>
#include <gtkmm/filefilter.h>
#include <glibmm/datetime.h>
#include <glibmm/convert.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/urilauncher.h>
#include <pangomm/layout.h>

#include <glibmm/i18n.h>

#include "modules/file.h"
#include "window.h"

namespace Notepad {

namespace {

// adw_alert_dialog_choose() is plain-C/GAsyncReadyCallback (no gtkmm
// binding exists for libadwaita, see the plan's Architecture correction
// note) so it needs a C-function trampoline bridging into a heap-allocated
// std::function, freed once the response arrives.
void alert_dialog_choose_trampoline(GObject *source, GAsyncResult *result,
                                     gpointer user_data) {
  auto *callback = static_cast<std::function<void(const Glib::ustring &)> *>(user_data);
  const char *response = adw_alert_dialog_choose_finish(ADW_ALERT_DIALOG(source), result);
  (*callback)(response ? response : "");
  delete callback;
}

} // namespace

Commands::Commands(Window &window, Editor &editor, AppState &state,
                    const Settings &settings)
    : window_(window), editor_(editor), state_(state), settings_(settings) {}

void Commands::register_actions() {
  auto *win = window_.gtk_window();

  win->add_action("new", sigc::mem_fun(*this, &Commands::file_new));
  win->add_action("open", sigc::mem_fun(*this, &Commands::file_open));
  win->add_action("save", [this]() { file_save(); });
  win->add_action("save-as", [this]() { file_save_as(); });
  win->add_action("print", sigc::mem_fun(*this, &Commands::file_print));
  // win.page-setup is intentionally never registered: GtkPrintOperation's
  // PRINT_DIALOG already includes page setup, so legacy's separate
  // FilePageSetup command/dialog has no equivalent here (documented
  // consolidation, see the plan's Phase 6 notes).

  win->add_action("undo", [this]() { editor_.view().get_buffer()->undo(); });
  win->add_action("redo", [this]() { editor_.view().get_buffer()->redo(); });
  win->add_action("cut", [this]() {
    editor_.view().get_buffer()->cut_clipboard(editor_.view().get_clipboard());
  });
  win->add_action("copy", [this]() {
    editor_.view().get_buffer()->copy_clipboard(editor_.view().get_clipboard());
  });
  win->add_action("paste", [this]() {
    editor_.view().get_buffer()->paste_clipboard(editor_.view().get_clipboard());
  });
  win->add_action("delete", [this]() { editor_.view().get_buffer()->erase_selection(); });
  win->add_action("select-all", [this]() {
    auto buffer = editor_.view().get_buffer();
    buffer->select_range(buffer->begin(), buffer->end());
  });
  win->add_action("time-date", sigc::mem_fun(*this, &Commands::edit_time_date));

  win->add_action("check-updates", []() {
    auto launcher = Gtk::UriLauncher::create(
        "https://github.com/chiddekel/gnome-notepad/releases/latest");
    launcher->launch({}, {});
  });
}

void Commands::confirm_discard(std::function<void()> on_proceed) {
  bool has_unsaved = state_.modified && !(state_.file_path.empty() && editor_.get_text().empty());
  if (!has_unsaved) {
    on_proceed();
    return;
  }

  Glib::ustring name = state_.file_path.empty()
                            ? Glib::ustring(_("Untitled"))
                            : Glib::filename_display_basename(state_.file_path.raw());
  Glib::ustring body =
      Glib::ustring::compose(_("If you don't save, changes to “%1” will be lost."), name);

  AdwDialog *dialog = adw_alert_dialog_new(_("Save changes?"), body.c_str());
  auto *alert = ADW_ALERT_DIALOG(dialog);
  adw_alert_dialog_add_responses(alert, "cancel", _("_Cancel"), "discard", _("_Discard"),
                                  "save", _("_Save"), nullptr);
  adw_alert_dialog_set_response_appearance(alert, "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_response_appearance(alert, "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(alert, "save");
  adw_alert_dialog_set_close_response(alert, "cancel");

  auto *callback = new std::function<void(const Glib::ustring &)>(
      [this, on_proceed](const Glib::ustring &response) {
        if (response == "save") {
          file_save(on_proceed);
        } else if (response == "discard") {
          on_proceed();
        }
        // "cancel" (or the dialog being dismissed): do nothing.
      });

  adw_alert_dialog_choose(alert, GTK_WIDGET(window_.gtk_window()->gobj()), nullptr,
                           alert_dialog_choose_trampoline, callback);
}

void Commands::file_new() {
  confirm_discard([this]() {
    editor_.set_text("");
    state_.file_path.clear();
    state_.modified = false;
    state_.encoding = Encoding::UTF8;
    state_.line_ending = static_cast<LineEnding>(
        settings_.gio_settings()->get_enum("default-line-ending"));
    window_.refresh();
  });
}

void Commands::file_open() {
  confirm_discard([this]() {
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title(_("Open File"));

    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    auto text_filter = Gtk::FileFilter::create();
    text_filter->set_name(_("Text Documents"));
    text_filter->add_pattern("*.txt");
    filters->append(text_filter);
    auto all_filter = Gtk::FileFilter::create();
    all_filter->set_name(_("All Files"));
    all_filter->add_pattern("*");
    filters->append(all_filter);
    dialog->set_filters(filters);

    dialog->open(*window_.gtk_window(),
                 [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
                   try {
                     auto file = dialog->open_finish(result);
                     if (file) {
                       open_path(file->get_path());
                     }
                   } catch (const Glib::Error &) {
                     // Cancelled or failed; nothing to do.
                   }
                 });
  });
}

void Commands::file_save(std::function<void()> on_done) {
  if (state_.file_path.empty()) {
    file_save_as(on_done);
    return;
  }
  Glib::ustring error;
  if (save_file(state_.file_path, editor_.get_text(), state_.encoding,
                state_.line_ending, error)) {
    state_.modified = false;
    update_recent_files(state_.recent_files, state_.file_path);
    settings_.save_recent_files(state_.recent_files);
  }
  window_.refresh();
  if (on_done) {
    on_done();
  }
}

void Commands::file_save_as(std::function<void()> on_done) {
  auto dialog = Gtk::FileDialog::create();
  dialog->set_title(_("Save As"));
  dialog->set_initial_name(state_.file_path.empty()
                                ? _("Untitled.txt")
                                : Glib::filename_display_basename(state_.file_path.raw()).raw());

  dialog->save(*window_.gtk_window(),
               [this, dialog, on_done](const Glib::RefPtr<Gio::AsyncResult> &result) {
                 try {
                   auto file = dialog->save_finish(result);
                   if (file) {
                     Glib::ustring path = file->get_path();
                     Glib::ustring error;
                     if (save_file(path, editor_.get_text(), state_.encoding,
                                    state_.line_ending, error)) {
                       state_.file_path = path;
                       state_.modified = false;
                       update_recent_files(state_.recent_files, path);
                       settings_.save_recent_files(state_.recent_files);
                     }
                   }
                 } catch (const Glib::Error &) {
                   // Cancelled or failed; nothing to do.
                 }
                 window_.refresh();
                 if (on_done) {
                   on_done();
                 }
               });
}

void Commands::file_print() {
  auto op = Gtk::PrintOperation::create();
  op->set_job_name(state_.file_path.empty()
                        ? Glib::ustring(_("Untitled"))
                        : Glib::filename_display_basename(state_.file_path.raw()));

  // GtkPrintOperation/Pango handle pagination and page setup themselves,
  // replacing legacy's ~60-line hand-rolled GDI StartDocW/StartPage
  // pagination loop -- the biggest LOC reduction in the port (see plan
  // Phase 6). Every page renders the same full-document Pango::Layout,
  // translated so Cairo's per-page clip only shows that page's slice.
  auto layout_holder = std::make_shared<Glib::RefPtr<Pango::Layout>>();
  auto page_height_holder = std::make_shared<double>(1.0);

  op->signal_begin_print().connect(
      [this, op, layout_holder,
       page_height_holder](const Glib::RefPtr<Gtk::PrintContext> &context) {
        auto layout = context->create_pango_layout();
        layout->set_font_description(Pango::FontDescription(
            Glib::ustring::compose("%1 %2", state_.font_name, state_.font_size)));
        layout->set_width(static_cast<int>(context->get_width() * PANGO_SCALE));
        layout->set_wrap(Pango::WrapMode::WORD_CHAR);
        layout->set_text(editor_.get_text());

        int text_width_px = 0, text_height_px = 0;
        layout->get_pixel_size(text_width_px, text_height_px);

        double page_height = context->get_height();
        int pages = std::max(
            1, static_cast<int>(std::ceil(text_height_px / page_height)));

        *layout_holder = layout;
        *page_height_holder = page_height;
        op->set_n_pages(pages);
      });

  op->signal_draw_page().connect(
      [layout_holder, page_height_holder](
          const Glib::RefPtr<Gtk::PrintContext> &context, int page_nr) {
        auto cr = context->get_cairo_context();
        cr->save();
        cr->translate(0, -page_nr * (*page_height_holder));
        (*layout_holder)->show_in_cairo_context(cr);
        cr->restore();
      });

  try {
    op->run(Gtk::PrintOperation::Action::PRINT_DIALOG, *window_.gtk_window());
  } catch (const Glib::Error &) {
    // Print dialog cancelled or failed; nothing to do.
  }
}

void Commands::edit_time_date() {
  auto buffer = editor_.view().get_buffer();
  buffer->erase_selection();
  buffer->insert_at_cursor(Glib::DateTime::create_now_local().format("%I:%M %p %m/%d/%Y"));
}

void Commands::open_path(const Glib::ustring &path) {
  LoadResult result = load_file(path);
  if (!result.success) {
    return; // Error dialog surfacing is Phase 7 (dialog.cpp)
  }
  editor_.set_text(result.text);
  state_.file_path = path;
  state_.encoding = result.encoding;
  state_.line_ending = result.line_ending;
  state_.modified = false;
  update_recent_files(state_.recent_files, path);
  settings_.save_recent_files(state_.recent_files);
  window_.refresh();
}

} // namespace Notepad
