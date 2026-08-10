#pragma once

#include <cairomm/context.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

#include "core/enums.h"

namespace Notepad {

// Background image rendering behind the editor, replacing legacy-notepad's
// modules/background.* (GDI+ Image/Graphics/ImageAttributes). draw()'s
// 13-mode tile/stretch/fit/fill/anchor positioning math (ported verbatim
// from PaintBackground()) lives in core/bg-geometry.h's compute_blit_rects()
// -- pure geometry, independent of the graphics API and unit-tested without
// a Cairo context. draw() just blits whatever rects that returns:
// gdk_cairo_set_source_pixbuf() + Cairo::Context::paint_with_alpha() instead
// of Gdiplus::Graphics::DrawImage(). Legacy's manual double-buffering in
// UpdateBackgroundBitmap() (CreateCompatibleDC/BitBlt) is dropped entirely
// -- unnecessary under GTK4's compositor-backed rendering.
class Background {
public:
  bool load_image(const Glib::ustring &path, Glib::ustring &error_message);
  void clear_image();
  bool has_image() const { return static_cast<bool>(image_); }

  void draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height,
            BgPosition position, double opacity) const;

private:
  Glib::RefPtr<Gdk::Pixbuf> image_;
};

} // namespace Notepad
