#include "background.h"

#include <gdk/gdk.h> // gdk_cairo_set_source_pixbuf() -- no gtkmm wrapper exists
#include <glibmm/error.h>

#include "core/bg-geometry.h"

namespace Notepad {

bool Background::load_image(const Glib::ustring &path, Glib::ustring &error_message) {
  try {
    image_ = Gdk::Pixbuf::create_from_file(path.raw());
    return true;
  } catch (const Glib::Error &error) {
    image_.reset();
    error_message = error.what();
    return false;
  }
}

void Background::clear_image() { image_.reset(); }

void Background::draw(const Cairo::RefPtr<Cairo::Context> &cr, int width,
                       int height, BgPosition position, double opacity) const {
  if (!image_) {
    return;
  }
  double img_w = image_->get_width();
  double img_h = image_->get_height();
  if (img_w <= 0 || img_h <= 0) {
    return;
  }

  auto blit = [&](double x, double y, double w, double h) {
    cr->save();
    cr->translate(x, y);
    if (w != img_w || h != img_h) {
      cr->scale(w / img_w, h / img_h);
    }
    // gdk_cairo_set_source_pixbuf() is deprecated as of GTK 4.20 in favor
    // of a GdkTexture/Gtk::Snapshot-based rendering path, but it's still
    // fully functional and the direct Cairo::ImageSurface pixel-format
    // conversion (premultiplied ARGB32 vs. GdkPixbuf's row-major RGBA)
    // isn't worth hand-rolling here without a way to visually verify it.
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_cairo_set_source_pixbuf(cr->cobj(), image_->gobj(), 0, 0);
    G_GNUC_END_IGNORE_DEPRECATIONS
    cr->paint_with_alpha(opacity);
    cr->restore();
  };

  for (const auto &rect : compute_blit_rects(position, width, height, img_w, img_h)) {
    blit(rect.x, rect.y, rect.w, rect.h);
  }
}

} // namespace Notepad
