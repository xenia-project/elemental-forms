/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_RADIO_BUTTON_H_
#define EL_ELEMENTS_RADIO_BUTTON_H_

#include "el/element.h"
#include "el/elements/parts/base_radio_check_box.h"

namespace el {
namespace elements {

// A button which unselects other radiobuttons of the same group number when
// clicked.
// For a labeled radio button, use a LabelContainer containing a RadioButton.
class RadioButton : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(RadioButton, BaseRadioCheckBox);
  static void RegisterInflater();

  RadioButton() {
    set_background_skin(TBIDC("RadioButton"), InvokeInfo::kNoCallbacks);
  }
};

}  // namespace elements
namespace dsl {

struct RadioButtonNode : public ElementNode<RadioButtonNode> {
  using R = RadioButtonNode;
  RadioButtonNode() : ElementNode("RadioButton") {}
  //
  R& value(bool value) {
    set("value", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_RADIO_BUTTON_H_
