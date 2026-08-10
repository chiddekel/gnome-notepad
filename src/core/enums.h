#pragma once

#include <glibmm/ustring.h>

namespace Notepad {

enum class Encoding { UTF8, UTF8BOM, UTF16LE, UTF16BE, ANSI };

enum class LineEnding { CRLF, LF, CR };

enum class BgPosition {
  TopLeft,
  TopCenter,
  TopRight,
  CenterLeft,
  Center,
  CenterRight,
  BottomLeft,
  BottomCenter,
  BottomRight,
  Tile,
  Stretch,
  Fit,
  Fill,
};

enum class ColorScheme { System, Light, Dark };

// GSettings enum keys are stored by nick (string), not by integer value, so
// these round-trip LineEnding/BgPosition/ColorScheme to/from the nicks
// declared in data/io.github.chiddekel.Notepad.gschema.xml.
Glib::ustring to_nick(LineEnding value);
LineEnding line_ending_from_nick(const Glib::ustring &nick);

Glib::ustring to_nick(BgPosition value);
BgPosition bg_position_from_nick(const Glib::ustring &nick);

Glib::ustring to_nick(ColorScheme value);
ColorScheme color_scheme_from_nick(const Glib::ustring &nick);

} // namespace Notepad
