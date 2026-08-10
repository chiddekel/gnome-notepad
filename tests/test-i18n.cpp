#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <libintl.h>
#include <string>

#include "core/i18n.h"

using namespace Notepad;

namespace {

void test_apply_language_override_sets_and_clears_env() {
  apply_language_override("pl");
  const char *language = std::getenv("LANGUAGE");
  assert(language != nullptr);
  assert(std::string(language) == "pl");

  apply_language_override("");
  assert(std::getenv("LANGUAGE") == nullptr);
  std::puts("test_apply_language_override_sets_and_clears_env: OK");
}

void test_is_rtl_language() {
  assert(is_rtl_language("ar"));
  assert(!is_rtl_language("en"));
  assert(!is_rtl_language("pl"));
  assert(!is_rtl_language(""));
  std::puts("test_is_rtl_language: OK");
}

// Regression test for the bug this file was added to catch: flatpak-builder
// splits /app/share/locale into a separate .Locale extension by default
// (separate-locales), which never ships with our standalone-bundle
// distribution -- so notepad.mo silently never resolves and every
// translated string falls back to English regardless of the language
// switcher. This exercises the exact same bindtextdomain/textdomain/
// LANGUAGE-env path main.cpp and Window::language_action_ use, against
// the .mo files this build actually compiled and installed (via
// NOTEPAD_TEST_LOCALEDIR, pointed at the build's own locale output --
// see tests/meson.build), so a regression in either the C++ logic or the
// build's install step fails this test either way.
void test_gettext_resolves_installed_catalog() {
  const char *localedir = std::getenv("NOTEPAD_TEST_LOCALEDIR");
  assert(localedir != nullptr &&
         "NOTEPAD_TEST_LOCALEDIR must be set by the test harness");

  bindtextdomain("notepad", localedir);
  bind_textdomain_codeset("notepad", "UTF-8");
  textdomain("notepad");

  apply_language_override("pl");
  const char *translated = gettext("All Files");
  assert(std::string(translated) == "Wszystkie pliki");

  apply_language_override("");
  std::puts("test_gettext_resolves_installed_catalog: OK");
}

} // namespace

int main() {
  test_apply_language_override_sets_and_clears_env();
  test_is_rtl_language();
  test_gettext_resolves_installed_catalog();
  return 0;
}
