#pragma once

#include <giomm/settings.h>
#include <glibmm/refptr.h>

#include "core/app-state.h"

namespace Notepad {

// GSettings-backed persistence, replacing legacy-notepad's
// modules/settings.cpp (Registry key HKCU\Software\LegacyNotepad). See
// data/io.github.chiddekel.Notepad.gschema.xml for the schema.
class Settings {
public:
  Settings();

  // Populates every persisted AppState field from GSettings. Session-only
  // fields (file_path, modified, find_text, replace_text) are left as
  // AppState's own defaults.
  void load_into(AppState &state) const;

  // Mirrors legacy's SaveFontSettings(): font + always-on-top.
  void save_font(const AppState &state) const;

  // Mirrors legacy's SaveWindowSettings(): size + maximized state.
  // (window-x/window-y are intentionally not persisted, see the schema.)
  void save_window(const AppState &state) const;

  void save_editor_prefs(const AppState &state) const; // word-wrap, zoom, statusbar
  void save_color_scheme(ColorScheme scheme) const;
  void save_default_line_ending(LineEnding ending) const;
  void save_background(const BackgroundSettings &background) const;
  void save_recent_files(const std::deque<Glib::ustring> &recent_files) const;
  void save_language_override(const Glib::ustring &language) const;

  Glib::RefPtr<Gio::Settings> gio_settings() const { return settings_; }

private:
  Glib::RefPtr<Gio::Settings> settings_;
};

} // namespace Notepad
