/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Seger�s and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/label.h"
#include "tb/parsing/element_inflater.h"
#include "tb/text/font_face.h"

namespace tb {
namespace elements {

ElementString::ElementString() = default;

int ElementString::GetWidth(Element* element) {
  return element->GetFont()->GetStringWidth(m_text);
}

int ElementString::GetHeight(Element* element) {
  return element->GetFont()->height();
}

void ElementString::Paint(Element* element, const Rect& rect,
                          const Color& color) {
  auto font = element->GetFont();
  int string_w = GetWidth(element);

  int x = rect.x;
  if (m_text_align == TextAlign::kRight) {
    x += rect.w - string_w;
  } else if (m_text_align == TextAlign::kCenter) {
    x += std::max(0, (rect.w - string_w) / 2);
  }
  int y = rect.y + (rect.h - GetHeight(element)) / 2;

  if (string_w <= rect.w) {
    font->DrawString(x, y, color, m_text);
  } else {
    // There's not enough room for the entire string
    // so cut it off and end with ellipsis (...)

    // const char *end = "�"; // 2026 HORIZONTAL ELLIPSIS
    // Some fonts seem to render ellipsis a lot uglier than three dots.
    const char* end = "...";

    int endw = font->GetStringWidth(end);
    int startw = 0;
    int startlen = 0;
    while (m_text[startlen]) {
      int new_startw = font->GetStringWidth(m_text, startlen);
      if (new_startw + endw > rect.w) {
        break;
      }
      startw = new_startw;
      startlen++;
    }
    startlen = std::max(0, startlen - 1);
    font->DrawString(x, y, color, m_text, startlen);
    font->DrawString(x + startw, y, color, end);
  }
}

void Label::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Label, Value::Type::kString, ElementZ::kTop);
}

Label::Label() { SetSkinBg(TBIDC("Label"), InvokeInfo::kNoCallbacks); }

void Label::OnInflate(const parsing::InflateInfo& info) {
  if (const char* text_align =
          info.node->GetValueString("text-align", nullptr)) {
    SetTextAlign(from_string(text_align, GetTextAlign()));
  }
  Element::OnInflate(info);
}

void Label::SetText(const char* text) {
  if (m_text.m_text.compare(text) == 0) return;
  m_cached_text_width = kTextWidthCacheNeedsUpdate;
  Invalidate();
  InvalidateLayout(InvalidationMode::kRecursive);
  m_text.SetText(text);
}

void Label::SetSqueezable(bool squeezable) {
  if (squeezable == m_squeezable) return;
  m_squeezable = squeezable;
  Invalidate();
  InvalidateLayout(InvalidationMode::kRecursive);
}

PreferredSize Label::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps;
  if (m_cached_text_width == kTextWidthCacheNeedsUpdate) {
    m_cached_text_width = m_text.GetWidth(this);
  }
  ps.pref_w = m_cached_text_width;
  ps.pref_h = ps.min_h = m_text.GetHeight(this);
  // If gravity pull both up and down, use default max_h (grow as much as
  // possible).
  // Otherwise it makes sense to only accept one line height.
  if (!(any(GetGravity() & Gravity::kTop) &&
        any(GetGravity() & Gravity::kBottom))) {
    ps.max_h = ps.pref_h;
  }
  if (!m_squeezable) {
    ps.min_w = ps.pref_w;
  }
  return ps;
}

void Label::OnFontChanged() {
  m_cached_text_width = kTextWidthCacheNeedsUpdate;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Label::OnPaint(const PaintProps& paint_props) {
  m_text.Paint(this, GetPaddingRect(), paint_props.text_color);
}

}  // namespace elements
}  // namespace tb