#include <cassert>
#include <cstdio>

#include "core/bg-geometry.h"

using namespace Notepad;

namespace {

// 200x100 container, 50x40 image -- image fits inside the container on
// every axis, so the 9 anchor modes each place it verbatim, unscaled.
constexpr double kW = 200, kH = 100, kImgW = 50, kImgH = 40;

void assert_single_rect(BgPosition position, double x, double y, double w,
                         double h) {
  auto rects = compute_blit_rects(position, kW, kH, kImgW, kImgH);
  assert(rects.size() == 1);
  assert(rects[0].x == x);
  assert(rects[0].y == y);
  assert(rects[0].w == w);
  assert(rects[0].h == h);
}

void test_anchor_modes() {
  assert_single_rect(BgPosition::TopLeft, 0, 0, kImgW, kImgH);
  assert_single_rect(BgPosition::TopCenter, 75, 0, kImgW, kImgH);
  assert_single_rect(BgPosition::TopRight, 150, 0, kImgW, kImgH);
  assert_single_rect(BgPosition::CenterLeft, 0, 30, kImgW, kImgH);
  assert_single_rect(BgPosition::Center, 75, 30, kImgW, kImgH);
  assert_single_rect(BgPosition::CenterRight, 150, 30, kImgW, kImgH);
  assert_single_rect(BgPosition::BottomLeft, 0, 60, kImgW, kImgH);
  assert_single_rect(BgPosition::BottomCenter, 75, 60, kImgW, kImgH);
  assert_single_rect(BgPosition::BottomRight, 150, 60, kImgW, kImgH);
  std::puts("test_anchor_modes: OK");
}

void test_stretch_fills_container_ignoring_aspect_ratio() {
  assert_single_rect(BgPosition::Stretch, 0, 0, kW, kH);
  std::puts("test_stretch_fills_container_ignoring_aspect_ratio: OK");
}

void test_fit_scales_to_smaller_axis_and_letterboxes() {
  // scale = min(200/50, 100/40) = min(4, 2.5) = 2.5 -> 125x100, height-fit,
  // horizontally letterboxed (pillarboxed) by (200-125)/2 on each side.
  assert_single_rect(BgPosition::Fit, 37.5, 0, 125, 100);
  std::puts("test_fit_scales_to_smaller_axis_and_letterboxes: OK");
}

void test_fill_scales_to_larger_axis_and_overflows() {
  // scale = max(200/50, 100/40) = max(4, 2.5) = 4 -> 200x160, width-fit,
  // overflowing vertically by (100-160)/2 = -30 on each side (cropped by
  // the caller's clip, same as legacy PaintBackground()).
  assert_single_rect(BgPosition::Fill, 0, -30, 200, 160);
  std::puts("test_fill_scales_to_larger_axis_and_overflows: OK");
}

void test_tile_covers_container_with_unscaled_repeats() {
  // 200/50 = 4 columns exactly, 100/40 = 2.5 -> 3 rows (0, 40, 80).
  auto rects = compute_blit_rects(BgPosition::Tile, kW, kH, kImgW, kImgH);
  assert(rects.size() == 4 * 3);
  assert(rects.front().x == 0);
  assert(rects.front().y == 0);
  assert(rects.back().x == 150);
  assert(rects.back().y == 80);
  for (const auto &rect : rects) {
    assert(rect.w == kImgW);
    assert(rect.h == kImgH);
  }
  std::puts("test_tile_covers_container_with_unscaled_repeats: OK");
}

void test_tile_with_uneven_division_still_covers_container() {
  // 90/50 -> 2 columns (0, 50; 100 not < 90), 70/40 -> 2 rows (0, 40; 80
  // not < 70). The last row/column overflows the container -- intentional,
  // same as legacy: the caller clips to the widget's own draw area.
  auto rects = compute_blit_rects(BgPosition::Tile, 90, 70, kImgW, kImgH);
  assert(rects.size() == 2 * 2);
  std::puts("test_tile_with_uneven_division_still_covers_container: OK");
}

} // namespace

int main() {
  test_anchor_modes();
  test_stretch_fills_container_ignoring_aspect_ratio();
  test_fit_scales_to_smaller_axis_and_letterboxes();
  test_fill_scales_to_larger_axis_and_overflows();
  test_tile_covers_container_with_unscaled_repeats();
  test_tile_with_uneven_division_still_covers_container();
  return 0;
}
