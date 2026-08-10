#include <cstdlib>
#include <giomm/init.h>
#include <giomm/settings.h>
#include <libintl.h>

#include "application.h"
#include "config.h"
#include "core/i18n.h"

int main(int argc, char *argv[]) {
  // Gio::Settings::create() below needs Gio's GObject types registered,
  // which normally happens implicitly inside Gtk::Application's own
  // startup -- but we're calling it before that exists yet.
  Gio::init();

  // Apply a persisted language override (if any) before gettext resolves
  // the locale, using the same helper Window::register_actions() calls to
  // live-switch languages at runtime -- see core/i18n.h.
  auto settings = Gio::Settings::create(APP_ID);
  Glib::ustring language_override = settings->get_string("language-override");
  Notepad::apply_language_override(language_override);

  // LOCALEDIR (config.h) is the *installed* prefix's locale directory --
  // when running straight out of the build tree (see deliver.sh/README's
  // "run without installing"), nothing was ever installed there, so
  // gettext() would silently find no .mo catalogs and every language
  // would render as untranslated English. NOTEPAD_LOCALEDIR lets an
  // uninstalled run point at build/po instead, where meson's i18n module
  // actually compiles the catalogs to <lang>/LC_MESSAGES/notepad.mo.
  const char *localedir_override = std::getenv("NOTEPAD_LOCALEDIR");
  bindtextdomain(GETTEXT_PACKAGE, localedir_override ? localedir_override : LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  auto app = Notepad::Application::create();
  return app->run(argc, argv);
}
