#include "settings.h"

#include "config.h"

namespace Notepad {

Settings::Settings() : settings_(Gio::Settings::create(APP_ID)) {}

void Settings::load_into(AppState &state) const {
  state.window_width = settings_->get_int("window-width");
  state.window_height = settings_->get_int("window-height");
  state.window_maximized = settings_->get_boolean("window-maximized");

  state.word_wrap = settings_->get_boolean("word-wrap");
  state.zoom_level = settings_->get_int("zoom-level");
  state.show_statusbar = settings_->get_boolean("show-statusbar");
  state.color_scheme = static_cast<ColorScheme>(settings_->get_enum("color-scheme"));

  state.font_name = settings_->get_string("font-name");
  state.font_size = settings_->get_int("font-size");
  state.font_weight = settings_->get_int("font-weight");
  state.font_italic = settings_->get_boolean("font-italic");
  state.font_underline = settings_->get_boolean("font-underline");

  state.always_on_top = settings_->get_boolean("always-on-top");

  state.background.enabled = settings_->get_boolean("background-enabled");
  state.background.image_path = settings_->get_string("background-image-path");
  state.background.position =
      static_cast<BgPosition>(settings_->get_enum("background-position"));
  state.background.opacity = settings_->get_double("background-opacity");

  state.recent_files.clear();
  for (const auto &path : settings_->get_string_array("recent-files")) {
    state.recent_files.push_back(path);
  }

  state.language_override = settings_->get_string("language-override");

  // default-line-ending seeds the encoding-independent default for *new*
  // files; it does not overwrite the line ending of a file already loaded
  // into AppState, so it is applied by file.cpp on FileNew, not here.
}

void Settings::save_font(const AppState &state) const {
  settings_->set_string("font-name", state.font_name);
  settings_->set_int("font-size", state.font_size);
  settings_->set_int("font-weight", state.font_weight);
  settings_->set_boolean("font-italic", state.font_italic);
  settings_->set_boolean("font-underline", state.font_underline);
  settings_->set_boolean("always-on-top", state.always_on_top);
}

void Settings::save_window(const AppState &state) const {
  settings_->set_int("window-width", state.window_width);
  settings_->set_int("window-height", state.window_height);
  settings_->set_boolean("window-maximized", state.window_maximized);
}

void Settings::save_editor_prefs(const AppState &state) const {
  settings_->set_boolean("word-wrap", state.word_wrap);
  settings_->set_int("zoom-level", state.zoom_level);
  settings_->set_boolean("show-statusbar", state.show_statusbar);
}

void Settings::save_color_scheme(ColorScheme scheme) const {
  settings_->set_enum("color-scheme", static_cast<int>(scheme));
}

void Settings::save_default_line_ending(LineEnding ending) const {
  settings_->set_enum("default-line-ending", static_cast<int>(ending));
}

void Settings::save_background(const BackgroundSettings &background) const {
  settings_->set_boolean("background-enabled", background.enabled);
  settings_->set_string("background-image-path", background.image_path);
  settings_->set_enum("background-position", static_cast<int>(background.position));
  settings_->set_double("background-opacity", background.opacity);
}

void Settings::save_recent_files(
    const std::deque<Glib::ustring> &recent_files) const {
  std::vector<Glib::ustring> as_vector(recent_files.begin(), recent_files.end());
  settings_->set_string_array("recent-files", as_vector);
}

void Settings::save_language_override(const Glib::ustring &language) const {
  settings_->set_string("language-override", language);
}

} // namespace Notepad
