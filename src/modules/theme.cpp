#include "theme.h"

#include <adwaita.h>

namespace Notepad {

namespace {

AdwColorScheme to_adw_color_scheme(ColorScheme scheme) {
  switch (scheme) {
  case ColorScheme::Light:
    return ADW_COLOR_SCHEME_FORCE_LIGHT;
  case ColorScheme::Dark:
    return ADW_COLOR_SCHEME_FORCE_DARK;
  case ColorScheme::System:
    return ADW_COLOR_SCHEME_DEFAULT;
  }
  return ADW_COLOR_SCHEME_DEFAULT;
}

} // namespace

void Theme::apply_color_scheme(ColorScheme scheme) {
  adw_style_manager_set_color_scheme(adw_style_manager_get_default(),
                                      to_adw_color_scheme(scheme));
}

bool Theme::is_dark() {
  return adw_style_manager_get_dark(adw_style_manager_get_default());
}

} // namespace Notepad
