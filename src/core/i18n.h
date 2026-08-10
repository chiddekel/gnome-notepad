#pragma once

#include <glibmm/ustring.h>

namespace Notepad {

// Applies (or clears) the persisted UI-language override by rewriting the
// LANGUAGE environment variable and re-deriving LC_MESSAGES from it. GNU
// gettext re-reads LANGUAGE on every _()/gettext() call rather than caching
// it once at startup, so calling this at runtime is enough to make
// subsequent translation lookups return the new language -- callers still
// need to re-run any code that only called _() once, at construction time
// (see Window::refresh_translations() / Dialogs::refresh_translations()).
// nick empty means "no override, follow the system locale".
void apply_language_override(const Glib::ustring &nick);

// True for languages that read right-to-left, so callers can flip
// Gtk::Widget::set_default_direction() to match -- otherwise only resolved
// once, from the locale active at gtk_init time.
bool is_rtl_language(const Glib::ustring &nick);

} // namespace Notepad
