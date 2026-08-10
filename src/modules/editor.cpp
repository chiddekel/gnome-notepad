#include "editor.h"

#include <algorithm>
#include <gdkmm/display.h>
#include <gtk/gtk.h> // GTK_STYLE_PROVIDER_PRIORITY_APPLICATION (no gtkmm wrapper)
#include <gtkmm/styleprovider.h>
#include <gtkmm/textbuffer.h>

namespace Notepad {

namespace {
constexpr int kMinEffectiveFontSize = 8;
constexpr int kMaxEffectiveFontSize = 500;
constexpr const char *kEditorCssClass = "notepad-editor";
} // namespace

Editor::Editor(Gtk::TextView &view)
    : view_(view), font_css_(Gtk::CssProvider::create()),
      background_css_(Gtk::CssProvider::create()) {
  view_.add_css_class(kEditorCssClass);
  Gtk::StyleProvider::add_provider_for_display(
      Gdk::Display::get_default(), font_css_,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  Gtk::StyleProvider::add_provider_for_display(
      Gdk::Display::get_default(), background_css_,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

Glib::ustring Editor::get_text() const {
  return view_.get_buffer()->get_text();
}

void Editor::set_text(const Glib::ustring &text) {
  view_.get_buffer()->set_text(text);
}

std::pair<int, int> Editor::get_cursor_pos() const {
  auto buffer = view_.get_buffer();
  auto iter = buffer->get_iter_at_mark(buffer->get_insert());
  return {iter.get_line() + 1, iter.get_line_offset() + 1};
}

void Editor::apply_font(const AppState &state) {
  int effective_size = state.font_size * state.zoom_level / 100;
  effective_size = std::max(kMinEffectiveFontSize,
                             std::min(kMaxEffectiveFontSize, effective_size));

  Glib::ustring css = Glib::ustring::compose(
      ".%1 { font-family: \"%2\"; font-size: %3pt; font-weight: %4; "
      "font-style: %5; text-decoration-line: %6; }",
      kEditorCssClass, state.font_name, effective_size, state.font_weight,
      state.font_italic ? "italic" : "normal",
      state.font_underline ? "underline" : "none");
  font_css_->load_from_string(css);
}

void Editor::apply_word_wrap(bool enabled) {
  view_.set_wrap_mode(enabled ? Gtk::WrapMode::WORD_CHAR : Gtk::WrapMode::NONE);
}

void Editor::set_transparent_background(bool transparent) {
  Glib::ustring css =
      transparent
          ? Glib::ustring::compose(".%1, .%1 text { background-color: transparent; }",
                                    kEditorCssClass)
          : Glib::ustring();
  background_css_->load_from_string(css.empty() ? " " : css);
}

} // namespace Notepad
