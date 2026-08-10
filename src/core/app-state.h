#pragma once

#include <array>
#include <deque>
#include <glibmm/ustring.h>

#include "enums.h"

namespace Notepad {

inline constexpr int kZoomMin = 25;
inline constexpr int kZoomMax = 500;
inline constexpr int kZoomDefault = 100;
inline constexpr int kMinFontSize = 8;
inline constexpr int kMaxFontSize = 72;
inline constexpr std::size_t kMaxRecentFiles = 10;

// Fixed zoom steps, ported verbatim from legacy-notepad's commands.cpp.
extern const std::array<int, 14> kZoomSteps;

struct BackgroundSettings {
  bool enabled = false;
  Glib::ustring image_path;
  BgPosition position = BgPosition::Center;
  double opacity = 0.5;
};

// In-memory session/document state. Persisted fields round-trip through
// Notepad::Settings (GSettings); filePath/modified/findText/replaceText are
// session-only and never touch GSettings (see the plan's GSettings schema
// table).
struct AppState {
  Glib::ustring file_path;
  bool modified = false;
  Encoding encoding = Encoding::UTF8;
  LineEnding line_ending = LineEnding::LF;
  Glib::ustring find_text;
  Glib::ustring replace_text;

  bool word_wrap = false;
  int zoom_level = kZoomDefault;
  bool show_statusbar = true;
  ColorScheme color_scheme = ColorScheme::System;

  Glib::ustring font_name = "Monospace";
  int font_size = 11;
  int font_weight = 400;
  bool font_italic = false;
  bool font_underline = false;

  bool always_on_top = false;

  std::deque<Glib::ustring> recent_files;
  BackgroundSettings background;

  int window_width = 640;
  int window_height = 480;
  bool window_maximized = false;

  Glib::ustring language_override;
};

} // namespace Notepad
