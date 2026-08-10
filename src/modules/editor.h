#pragma once

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/textview.h>
#include <utility>

#include "core/app-state.h"

namespace Notepad {

// Wraps a Gtk::TextView/Gtk::TextBuffer, replacing legacy-notepad's
// modules/editor.* (RichEdit EM_STREAMIN/OUT, EM_GETSEL/EM_SETCHARFORMAT,
// destroy-and-recreate-on-wrap-toggle). DeleteWordBackward/Forward and the
// hand-built context menu / shift+wheel horizontal scroll from
// EditorSubclassProc are dropped entirely -- GtkTextView provides all three
// natively.
class Editor {
public:
  explicit Editor(Gtk::TextView &view);

  Glib::ustring get_text() const;
  void set_text(const Glib::ustring &text);

  // 1-based (line, column), matching legacy-notepad's GetCursorPos().
  std::pair<int, int> get_cursor_pos() const;

  // Font size is scaled by state.zoom_level, clamped to [8, 500] -- ported
  // verbatim from legacy-notepad's ApplyFont(). Text color is left to
  // Phase 8 (theme.cpp / Adw::StyleManager), not handled here.
  void apply_font(const AppState &state);

  // Legacy's ApplyZoom() just re-derives the effective size via ApplyFont();
  // same here.
  void apply_zoom(const AppState &state) { apply_font(state); }

  // A live gtk_text_view_set_wrap_mode() call -- no control recreation
  // needed, unlike the RichEdit original.
  void apply_word_wrap(bool enabled);

  // Makes the text view's own background see-through so a background
  // image (Phase 9) drawn behind it in a Gtk::Overlay is visible.
  void set_transparent_background(bool transparent);

  Gtk::TextView &view() { return view_; }

private:
  Gtk::TextView &view_;
  Glib::RefPtr<Gtk::CssProvider> font_css_;
  Glib::RefPtr<Gtk::CssProvider> background_css_;
};

} // namespace Notepad
