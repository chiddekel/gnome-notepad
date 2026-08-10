#include "core/bg-geometry.h"

#include <algorithm>

namespace Notepad {

std::vector<BlitRect> compute_blit_rects(BgPosition position, double width,
                                          double height, double img_w,
                                          double img_h) {
  switch (position) {
  case BgPosition::TopLeft:
    return {{0, 0, img_w, img_h}};
  case BgPosition::TopCenter:
    return {{(width - img_w) / 2.0, 0, img_w, img_h}};
  case BgPosition::TopRight:
    return {{width - img_w, 0, img_w, img_h}};
  case BgPosition::CenterLeft:
    return {{0, (height - img_h) / 2.0, img_w, img_h}};
  case BgPosition::Center:
    return {{(width - img_w) / 2.0, (height - img_h) / 2.0, img_w, img_h}};
  case BgPosition::CenterRight:
    return {{width - img_w, (height - img_h) / 2.0, img_w, img_h}};
  case BgPosition::BottomLeft:
    return {{0, height - img_h, img_w, img_h}};
  case BgPosition::BottomCenter:
    return {{(width - img_w) / 2.0, height - img_h, img_w, img_h}};
  case BgPosition::BottomRight:
    return {{width - img_w, height - img_h, img_w, img_h}};
  case BgPosition::Tile: {
    std::vector<BlitRect> rects;
    for (double y = 0; y < height; y += img_h) {
      for (double x = 0; x < width; x += img_w) {
        rects.push_back({x, y, img_w, img_h});
      }
    }
    return rects;
  }
  case BgPosition::Stretch:
    return {{0, 0, width, height}};
  case BgPosition::Fit: {
    double scale = std::min(width / img_w, height / img_h);
    double new_w = img_w * scale, new_h = img_h * scale;
    return {{(width - new_w) / 2.0, (height - new_h) / 2.0, new_w, new_h}};
  }
  case BgPosition::Fill: {
    double scale = std::max(width / img_w, height / img_h);
    double new_w = img_w * scale, new_h = img_h * scale;
    return {{(width - new_w) / 2.0, (height - new_h) / 2.0, new_w, new_h}};
  }
  }
  return {};
}

} // namespace Notepad
