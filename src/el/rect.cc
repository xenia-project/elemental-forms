/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "el/rect.h"
#include "el/util/math.h"
#include "el/util/string.h"

namespace el {

inline std::string to_string(const Point& value) {
  return util::format_string("%d %d", value.x, value.y);
}

inline std::string to_string(const Rect& value) {
  return util::format_string("%d %d %d %d", value.x, value.y, value.w, value.h);
}

bool Rect::intersects(const Rect& rect) const {
  if (empty() || rect.empty()) return false;
  if (x + w > rect.x && x < rect.x + rect.w && y + h > rect.y &&
      y < rect.y + rect.h)
    return true;
  return false;
}

Rect Rect::MoveIn(const Rect& bounding_rect) const {
  return Rect(util::ClampClipMax(x, bounding_rect.x,
                                 bounding_rect.x + bounding_rect.w - w),
              util::ClampClipMax(y, bounding_rect.y,
                                 bounding_rect.y + bounding_rect.h - h),
              w, h);
}

Rect Rect::CenterIn(const Rect& bounding_rect) const {
  return Rect((bounding_rect.w - w) / 2, (bounding_rect.h - h) / 2, w, h);
}

Rect Rect::Union(const Rect& rect) const {
  assert(!is_inside_out());
  assert(!rect.is_inside_out());

  if (empty()) return rect;
  if (rect.empty()) return *this;

  int minx = std::min(x, rect.x);
  int miny = std::min(y, rect.y);
  int maxx = x + w > rect.x + rect.w ? x + w : rect.x + rect.w;
  int maxy = y + h > rect.y + rect.h ? y + h : rect.y + rect.h;
  return Rect(minx, miny, maxx - minx, maxy - miny);
}

Rect Rect::Clip(const Rect& clip_rect) const {
  assert(!clip_rect.is_inside_out());
  Rect tmp;
  if (!intersects(clip_rect)) return tmp;
  tmp.x = std::max(x, clip_rect.x);
  tmp.y = std::max(y, clip_rect.y);
  tmp.w = std::min(x + w, clip_rect.x + clip_rect.w) - tmp.x;
  tmp.h = std::min(y + h, clip_rect.y + clip_rect.h) - tmp.y;
  return tmp;
}

}  // namespace el
