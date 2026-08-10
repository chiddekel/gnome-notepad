#pragma once

#include <vector>

#include "core/enums.h"

namespace Notepad {

// One placement for Background::draw()'s blit step: (x, y) is the
// top-left corner in container-local coordinates, (w, h) the size to
// scale the image to.
struct BlitRect {
  double x, y, w, h;
};

// Pure geometry for the 13 background positioning modes -- ported
// verbatim from legacy's PaintBackground() (see modules/background.h),
// independent of the graphics API so it's unit-testable without a Cairo
// context. Every mode returns exactly one rect except Tile, which repeats
// the image at its natural size across the full container.
std::vector<BlitRect> compute_blit_rects(BgPosition position, double width,
                                          double height, double img_w,
                                          double img_h);

} // namespace Notepad
