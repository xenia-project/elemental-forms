/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_CHECK_BOX_H_
#define EL_ELEMENTS_CHECK_BOX_H_

#include "el/element.h"
#include "el/elements/parts/base_radio_check_box.h"

namespace el {
namespace elements {

// A box toggling a check mark on click.
// For a labeled checkbox, use a LabelContainer containing a CheckBox.
class CheckBox : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(CheckBox, BaseRadioCheckBox);
  static void RegisterInflater();

  CheckBox() {
    set_background_skin(TBIDC("CheckBox"), InvokeInfo::kNoCallbacks);
  }
};

}  // namespace elements
namespace dsl {

struct CheckBoxNode : public ElementNode<CheckBoxNode> {
  using R = CheckBoxNode;
  CheckBoxNode() : ElementNode("CheckBox") {}
  //
  R& value(bool value) {
    set("value", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_CHECK_BOX_H_
