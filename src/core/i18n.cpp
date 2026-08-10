#include "i18n.h"

#include <clocale>
#include <cstdlib>

namespace Notepad {

void apply_language_override(const Glib::ustring &nick) {
  if (nick.empty()) {
    unsetenv("LANGUAGE");
  } else {
    setenv("LANGUAGE", nick.c_str(), 1);
  }
  // Only LC_MESSAGES is re-derived here, not LC_ALL/LC_NUMERIC/LC_TIME, so a
  // UI-language override never changes number/date formatting.
  setlocale(LC_MESSAGES, "");
}

bool is_rtl_language(const Glib::ustring &nick) { return nick == "ar"; }

} // namespace Notepad
