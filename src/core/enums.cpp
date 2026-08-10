#include "enums.h"

namespace Notepad {

Glib::ustring to_nick(LineEnding value) {
  switch (value) {
  case LineEnding::CRLF:
    return "crlf";
  case LineEnding::LF:
    return "lf";
  case LineEnding::CR:
    return "cr";
  }
  return "lf";
}

LineEnding line_ending_from_nick(const Glib::ustring &nick) {
  if (nick == "crlf")
    return LineEnding::CRLF;
  if (nick == "cr")
    return LineEnding::CR;
  return LineEnding::LF;
}

Glib::ustring to_nick(BgPosition value) {
  switch (value) {
  case BgPosition::TopLeft:
    return "top-left";
  case BgPosition::TopCenter:
    return "top-center";
  case BgPosition::TopRight:
    return "top-right";
  case BgPosition::CenterLeft:
    return "center-left";
  case BgPosition::Center:
    return "center";
  case BgPosition::CenterRight:
    return "center-right";
  case BgPosition::BottomLeft:
    return "bottom-left";
  case BgPosition::BottomCenter:
    return "bottom-center";
  case BgPosition::BottomRight:
    return "bottom-right";
  case BgPosition::Tile:
    return "tile";
  case BgPosition::Stretch:
    return "stretch";
  case BgPosition::Fit:
    return "fit";
  case BgPosition::Fill:
    return "fill";
  }
  return "center";
}

BgPosition bg_position_from_nick(const Glib::ustring &nick) {
  if (nick == "top-left")
    return BgPosition::TopLeft;
  if (nick == "top-center")
    return BgPosition::TopCenter;
  if (nick == "top-right")
    return BgPosition::TopRight;
  if (nick == "center-left")
    return BgPosition::CenterLeft;
  if (nick == "center-right")
    return BgPosition::CenterRight;
  if (nick == "bottom-left")
    return BgPosition::BottomLeft;
  if (nick == "bottom-center")
    return BgPosition::BottomCenter;
  if (nick == "bottom-right")
    return BgPosition::BottomRight;
  if (nick == "tile")
    return BgPosition::Tile;
  if (nick == "stretch")
    return BgPosition::Stretch;
  if (nick == "fit")
    return BgPosition::Fit;
  if (nick == "fill")
    return BgPosition::Fill;
  return BgPosition::Center;
}

Glib::ustring to_nick(ColorScheme value) {
  switch (value) {
  case ColorScheme::System:
    return "system";
  case ColorScheme::Light:
    return "light";
  case ColorScheme::Dark:
    return "dark";
  }
  return "system";
}

ColorScheme color_scheme_from_nick(const Glib::ustring &nick) {
  if (nick == "light")
    return ColorScheme::Light;
  if (nick == "dark")
    return ColorScheme::Dark;
  return ColorScheme::System;
}

} // namespace Notepad
