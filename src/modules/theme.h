#pragma once

#include "core/enums.h"

namespace Notepad {

// Thin wrapper around AdwStyleManager (no gtkmm binding exists for
// libadwaita, see the plan's Architecture correction note), replacing
// legacy-notepad's modules/theme.* almost entirely: the registry-based
// AppsUseLightTheme read, DwmSetWindowAttribute call, the undocumented
// uxtheme.dll dark-mode ordinal-export hack, and the RichEdit
// EM_SETBKGNDCOLOR/EM_SETCHARFORMAT dark-color calls all have zero
// equivalent here -- libadwaita themes every widget (including
// GtkTextView's background/text colors) automatically once the color
// scheme is set, no manual per-widget color pushes needed.
class Theme {
public:
  static void apply_color_scheme(ColorScheme scheme);
  static bool is_dark();
};

} // namespace Notepad
