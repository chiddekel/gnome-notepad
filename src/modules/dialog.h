#pragma once

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/revealer.h>
#include <gtkmm/searchentry.h>

#include "core/app-state.h"
#include "modules/editor.h"
#include "modules/settings.h"

namespace Notepad {

class Window; // full definition only needed in dialog.cpp

// Find/Replace bar, Go To Line, Font, About, and Window Transparency
// dialogs -- replaces legacy-notepad's modules/dialog.* (which hand-built
// every one of these as a raw #32770 window, including a from-scratch
// case-insensitive wraparound search algorithm that never actually used
// comdlg32's FindText/ReplaceText API). GtkTextIter::forward_search()/
// backward_search() already provide case-insensitive wraparound search
// natively, so that custom algorithm is dropped entirely.
class Dialogs {
public:
  Dialogs(Window &window, Editor &editor, AppState &state,
          const Settings &settings);

  void register_actions();

  // Owned find/replace bar widget; Window packs this into its toolbar view
  // as a bottom bar (hidden until win.find/win.replace reveals it).
  Gtk::Widget &find_bar() { return revealer_; }

  // Re-applies the Find/Replace bar's translated labels/placeholder, which
  // are otherwise only ever set once, in the constructor. Called by
  // Window::refresh_translations() after a live language switch.
  void refresh_translations();

private:
  void show_find_bar(bool with_replace);
  void hide_find_bar();
  void find_next();
  void find_previous();
  void do_replace();
  void do_replace_all();

  void show_goto_dialog();
  void show_font_dialog();
  void show_about_dialog();
  void show_transparency_dialog();

  Window &window_;
  Editor &editor_;
  AppState &state_;
  const Settings &settings_;

  Gtk::Revealer revealer_;
  Gtk::Box find_box_{Gtk::Orientation::VERTICAL, 4};
  Gtk::Box find_row_{Gtk::Orientation::HORIZONTAL, 6};
  Gtk::Box replace_row_{Gtk::Orientation::HORIZONTAL, 6};
  Gtk::SearchEntry search_entry_;
  Gtk::Entry replace_entry_;
  Gtk::Button find_prev_button_{"↑"};
  Gtk::Button find_next_button_{"↓"};
  // Labels set in the constructor body (dialog.cpp), not here -- xgettext
  // only extracts strings from po/POTFILES.in's listed .cpp files, not
  // headers, so a translatable _() call here would never actually be
  // extracted.
  Gtk::Button replace_button_;
  Gtk::Button replace_all_button_;
  Gtk::Button close_button_{"×"};
};

} // namespace Notepad
