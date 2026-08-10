#pragma once

#include <functional>
#include <glibmm/ustring.h>

#include "core/app-state.h"
#include "modules/editor.h"
#include "modules/settings.h"

namespace Notepad {

class Window; // full definition only needed in commands.cpp

// GAction callback implementations, replacing legacy-notepad's
// modules/commands.* (File/Edit/Format/View command handlers). Takes
// references to its collaborators instead of legacy's globals.
class Commands {
public:
  Commands(Window &window, Editor &editor, AppState &state,
           const Settings &settings);

  void register_actions();

  // Loads a file via file::load_file(), applies it to the editor/AppState,
  // refreshes the window, and records it as a recent file. Shared by
  // win.open and the window's drag & drop target.
  void open_path(const Glib::ustring &path);

  // Mirrors legacy's ConfirmDiscard(), but AdwAlertDialog is asynchronous
  // (adw_alert_dialog_choose()) where MessageBoxW was blocking, so this
  // takes a continuation instead of returning bool. Calls on_proceed()
  // immediately if there's nothing to lose; otherwise shows the
  // save/discard/cancel prompt and calls on_proceed() only for
  // save-then-succeeded or discard.
  void confirm_discard(std::function<void()> on_proceed);

private:
  void file_new();
  void file_open();
  void file_save(std::function<void()> on_done = {});
  void file_save_as(std::function<void()> on_done = {});
  void file_print();
  void edit_time_date();

  Window &window_;
  Editor &editor_;
  AppState &state_;
  const Settings &settings_;
};

} // namespace Notepad
