#pragma once

#include <adwaita.h>
#include <giomm/menu.h>
#include <giomm/simpleaction.h>
#include <glibmm/refptr.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/overlay.h>
#include <gtkmm/popovermenubar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

#include "core/app-state.h"
#include "modules/background.h"
#include "modules/commands.h"
#include "modules/dialog.h"
#include "modules/editor.h"
#include "modules/settings.h"

typedef struct _GtkApplication GtkApplication;

namespace Notepad {

// Owns a raw AdwApplicationWindow* (libadwaita has no stable gtkmm binding,
// see the Architecture correction note in the plan). Standard
// GtkApplicationWindow-level operations (present(), actions, close-request)
// go through the Glib::wrap()'d gtkmm view in gtk_window(); anything
// libadwaita-specific (content, header bar, toolbar view) uses the adw_* C
// API directly via ADW_APPLICATION_WINDOW()/ADW_TOOLBAR_VIEW() casts.
class Window {
public:
  Window(GtkApplication *app, AppState &state, const Settings &settings);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  Gtk::ApplicationWindow *gtk_window() const { return window_wrap_; }
  void present();

  // Captures current width/height/maximized back into state, mirroring
  // legacy-notepad's WM_DESTROY -> SaveWindowSettings() call site.
  void save_state(AppState &state) const;

  Editor &editor() { return editor_; }

  // Opens a file handed to the app externally (command-line argument or
  // GApplication::open, e.g. "Open With Notepad" from a file manager),
  // matching legacy-notepad's wWinMain command-line LoadFile() call.
  void open_external_path(const Glib::ustring &path);

  // Refreshes title/status/recent-files-menu after a command (Commands)
  // mutates AppState.
  void refresh();

private:
  void build_menu();
  void build_status_row();
  void register_actions();
  void setup_drop_target();

  void update_title();
  void update_status();
  void rebuild_recent_files_menu();

  // Re-applies translations to everything that only ever called _() once,
  // at construction time, so a live language switch is visible immediately:
  // rebuilds the menu models (build_menu()) and the Find/Replace bar's
  // labels (Dialogs::refresh_translations()), then refreshes title/status/
  // recent-files via refresh(). See the language action in
  // register_actions() and core/i18n.h.
  void refresh_translations();

  AppState &state_;
  const Settings &settings_;

  GtkWidget *window_; // AdwApplicationWindow*, floating ref owned by GtkApplication
  Gtk::ApplicationWindow *window_wrap_; // Glib::wrap()'d view; owned by GTK
  Background background_;
  Gtk::Overlay overlay_;
  Gtk::DrawingArea background_area_;
  Gtk::ScrolledWindow scroller_;
  Gtk::TextView editor_view_; // must outlive editor_ (declaration order)
  Editor editor_;
  Commands commands_; // constructed after editor_ (declaration order)
  Dialogs dialogs_;    // constructed after editor_ (declaration order)

  Gtk::MenuButton menu_button_;
  Glib::RefPtr<Gio::Menu> recent_files_menu_;

  // Phase EXTRA A: a classic Win32-style File/Edit/Format/View/Help menu row
  // under the header bar, built from the same submenu GMenuModels as
  // menu_button_'s hamburger menu (see build_menu()) -- same actions,
  // same state, just a second presentation.
  Gtk::PopoverMenuBar classic_menu_bar_;
  Gtk::Box status_box_{Gtk::Orientation::HORIZONTAL, 12};
  Gtk::Label status_position_;
  Gtk::Label status_encoding_;
  Gtk::Label status_line_ending_;
  Gtk::Label status_zoom_;

  Glib::RefPtr<Gio::SimpleAction> word_wrap_action_;
  Glib::RefPtr<Gio::SimpleAction> status_bar_action_;
  Glib::RefPtr<Gio::SimpleAction> always_on_top_action_;
  Glib::RefPtr<Gio::SimpleAction> dark_mode_action_;
  Glib::RefPtr<Gio::SimpleAction> background_position_action_;
  Glib::RefPtr<Gio::SimpleAction> language_action_;
  Glib::RefPtr<Gio::SimpleAction> has_recent_files_action_;
};

} // namespace Notepad
