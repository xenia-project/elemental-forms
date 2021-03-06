/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_SEPARATOR_H_
#define EL_ELEMENTS_SEPARATOR_H_

#include "el/element.h"

namespace el {
namespace elements {

// A element only showing a skin.
// It is disabled by default.
class Separator : public Element {
 public:
  TBOBJECT_SUBCLASS(Separator, Element);
  static void RegisterInflater();

  Separator();
};

}  // namespace elements
namespace dsl {

struct SeparatorNode : public ElementNode<SeparatorNode> {
  using R = SeparatorNode;
  SeparatorNode() : ElementNode("Separator") {}
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_SEPARATOR_H_
