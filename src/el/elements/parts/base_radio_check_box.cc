/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/parts/base_radio_check_box.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {
namespace parts {

BaseRadioCheckBox::BaseRadioCheckBox() {
  set_focusable(true);
  set_click_by_key(true);
}

// static
void BaseRadioCheckBox::UpdateGroupElements(Element* new_leader) {
  assert(new_leader->value() && new_leader->group_id());

  // Find the group root element.
  Element* group = new_leader;
  while (group && !group->is_group_root() && group->parent()) {
    group = group->parent();
  }

  for (Element* child = group; child; child = child->GetNextDeep(group)) {
    if (child != new_leader && child->group_id() == new_leader->group_id()) {
      child->set_value(0);
    }
  }
}

void BaseRadioCheckBox::set_value(int value) {
  if (m_value == value) return;
  m_value = value;

  set_state(Element::State::kSelected, value ? true : false);

  Event ev(EventType::kChanged);
  InvokeEvent(std::move(ev));

  if (value && group_id()) UpdateGroupElements(this);
}

PreferredSize BaseRadioCheckBox::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  ps.min_w = ps.max_w = ps.pref_w;
  ps.min_h = ps.max_h = ps.pref_h;
  return ps;
}

bool BaseRadioCheckBox::OnEvent(const Event& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Toggle the value, if it's not a grouped element with value on.
    if (!(group_id() && value())) {
      set_value(!value());
    }
  }
  return Element::OnEvent(ev);
}

}  // namespace parts
}  // namespace elements
}  // namespace el
