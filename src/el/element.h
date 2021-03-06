/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENT_H_
#define EL_ELEMENT_H_

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "el/dsl.h"
#include "el/element_value.h"
#include "el/event.h"
#include "el/font_description.h"
#include "el/rect.h"
#include "el/skin.h"
#include "el/types.h"
#include "el/util/intrusive_list.h"
#include "el/util/object.h"

namespace el {

class Element;
class ElementListener;
class EventHandler;
class GenericStringItemSource;
class LongClickTimer;
namespace elements {
class Form;
namespace parts {
class Scroller;
}  // namespace parts
}  // namespace elements
namespace parsing {
struct InflateInfo;
}  // namespace parsing
namespace text {
class FontFace;
}  // namespace text

using util::kInvalidDimension;

enum class Align {
  kLeft,
  kTop,
  kRight,
  kBottom,
};
MAKE_ORDERED_ENUM_STRING_UTILS(Align, "left", "top", "right", "bottom");

// TextAlign specifies horizontal text alignment.
enum class TextAlign {
  kLeft,
  kRight,
  kCenter,
};
MAKE_ORDERED_ENUM_STRING_UTILS(TextAlign, "left", "right", "center");

// Element gravity (may be combined).
// Gravity gives hints about positioning and sizing preferences.
enum class Gravity {
  kNone = 0,
  kLeft = 1,
  kRight = 2,
  kTop = 4,
  kBottom = 8,

  kLeftRight = kLeft | kRight,
  kTopBottom = kTop | kBottom,
  kAll = kLeftRight | kTopBottom,
  kDefault = kLeft | kTop,
};
MAKE_ENUM_FLAG_COMBO(Gravity);
inline std::string to_string(Gravity value) {
  if (value == Gravity::kNone) {
    return "none";
  } else if (value == Gravity::kAll) {
    return "all";
  }
  std::string result;
  if (any(value & Gravity::kLeft)) {
    result += " left";
  }
  if (any(value & Gravity::kRight)) {
    result += " right";
  }
  if (any(value & Gravity::kTop)) {
    result += " top";
  }
  if (any(value & Gravity::kBottom)) {
    result += " bottom";
  }
  return result.substr(1);
}

enum class Axis {
  kX,  // Horizontal layout.
  kY,  // Vertical layout.
};
MAKE_ORDERED_ENUM_STRING_UTILS(Axis, "x", "y");

// Defines how the size in one axis depend on the other axis when a elements
// size is affected by constraints.
enum class SizeDependency {
  // No dependency (Faster layout).
  kNone = 0,
  // The width is dependant on the height. Additional layout pass may be
  // required.
  kWidthOnHeight = 1,
  // The height is dependant on the width. Additional layout pass may be
  // required.
  kHeightOnWidth = 2,
  // Both width and height are dependant on each other. Additional layout pass
  // may be required.
  kWidthAndHeight = kWidthOnHeight | kHeightOnWidth
};
MAKE_ENUM_FLAG_COMBO(SizeDependency);

// Contains size preferences for a Element.
// This is calculated during layout for each element from the current skin,
// element preferences and LayoutParams.
class PreferredSize {
 public:
  PreferredSize() = default;
  PreferredSize(int w, int h)
      : min_w(w), min_h(h), max_w(w), max_h(h), pref_w(w), pref_h(h) {}

  // The minimal preferred width and height.
  int min_w = 0, min_h = 0;
  // The maximum preferred width and height.
  int max_w = 10000, max_h = 10000;
  // The preferred width and height.
  int pref_w = 0, pref_h = 0;
  // The size dependency when size is affected by constraints.
  SizeDependency size_dependency = SizeDependency::kNone;
};

// Defines size preferences for a Element that are set on the element to
// override size preferences from skin and element.
class LayoutParams {
 public:
  static const int kUnspecified = util::kInvalidDimension;

  LayoutParams() = default;

  // Sets both min max and preferred width to the given width.
  void set_width(int width) { min_w = max_w = pref_w = width; }

  // Set both min max and preferred height to the given height.
  void set_height(int height) { min_h = max_h = pref_h = height; }

  // The minimal preferred width and height.
  int min_w = kUnspecified, min_h = kUnspecified;
  // The maximum preferred width and height.
  int max_w = kUnspecified, max_h = kUnspecified;
  // The preferred width and height.
  int pref_w = kUnspecified, pref_h = kUnspecified;
};

// Specifies size constraints used during size calculations.
class SizeConstraints {
 public:
  static const int kNoRestriction = 10000;

  // The available width and height. May be kNoRestriction which is a large
  // value.
  int available_w = kNoRestriction;
  int available_h = kNoRestriction;

  // No constraints.
  SizeConstraints() = default;
  // Constrains to the given width and height.
  SizeConstraints(int w, int h) : available_w(w), available_h(h) {}

  // Returns new constraints reduced by the given padding.
  SizeConstraints ConstrainByPadding(int horizontal_padding,
                                     int vertical_padding) const {
    return SizeConstraints(
        available_w == kNoRestriction ? kNoRestriction
                                      : available_w - horizontal_padding,
        available_h == kNoRestriction ? kNoRestriction
                                      : available_h - vertical_padding);
  }

  // Returns new constraints that are constrained by LayoutParams.
  SizeConstraints ConstrainByLayoutParams(const LayoutParams& lp) const {
    return SizeConstraints(ConstrainByLPMax(available_w, lp.min_w, lp.max_w),
                           ConstrainByLPMax(available_h, lp.min_h, lp.max_h));
  }

  bool operator==(const SizeConstraints& sc) const {
    return available_w == sc.available_w && available_h == sc.available_h;
  }

 private:
  int ConstrainByLPMax(int constraint, int lp_min, int lp_max) const {
    if (constraint == kNoRestriction) {
      return lp_max != LayoutParams::kUnspecified ? lp_max : kNoRestriction;
    }
    int ret = constraint;
    if (lp_min != LayoutParams::kUnspecified) {
      ret = std::max(ret, lp_min);
    }
    if (lp_max != LayoutParams::kUnspecified) {
      ret = std::min(ret, lp_max);
    }
    return ret;
  }
};

// Defines element z level, used with Element::SetZ, Element::AddChild.
enum class ElementZ {
  kTop,     // The toplevel (Visually drawn on top of everything else).
  kBottom,  // The bottomlevel (Visually drawn behind everything else).
};
MAKE_ORDERED_ENUM_STRING_UTILS(ElementZ, "top", "bottom");

// Defines element z level relative to another element, used with
// Element::AddChildRelative.
enum class ElementZRel {
  kBefore,  // Before the reference element (visually behind reference).
  kAfter,   // After the reference element (visually above reference).
};
MAKE_ORDERED_ENUM_STRING_UTILS(ElementZRel, "before", "after");

// Defines element visibility, used with Element::SetVisibility.
enum class Visibility {
  kVisible,    // Visible (default).
  kInvisible,  // Invisible, but layouted. Interaction disabled.
  kGone        // Invisible and no layout. Interaction disabled.
};
MAKE_ORDERED_ENUM_STRING_UTILS(Visibility, "visible", "invisible", "gone");

enum class InvokeInfo {
  kNormal,
  kNoCallbacks,
};

enum class FocusReason {
  kNavigation,  // Set focus by navigation (i.e. keyboard tab). This will scroll
                // to the element if needed.
  kPointer,     // Set focus by pointer (i.e. clicking).
  kUnknown,     // Set focus by anything else.
};

// Hit status return value for Element::GetHitStatus.
enum class HitStatus {
  kNoHit,          // The element was not hit.
  kHit,            // The element was hit, any child may be hit too.
  kHitNoChildren,  // The element was hit, no children should be hit.
};

// The base Element class.
// Make a subclass to implement UI controls.
// Each element has a background skin (no skin specified by default) which will
// be used to calculate the default size preferences and padding around the
// preferred content size.
//
// NOTE: When you subclass a element, use the TBOBJECT_SUBCLASS macro to define
// the type casting functions instead of implementing those manually.
class Element : public util::TypedObject,
                public util::IntrusiveListEntry<Element> {
 public:
  TBOBJECT_SUBCLASS(Element, util::TypedObject);
  static void RegisterInflater();

  Element();
  virtual ~Element();

  bool LoadFile(const char* filename);
  bool LoadData(const char* data_str, size_t data_length = std::string::npos);
  bool LoadData(std::string data_str) {
    return LoadData(data_str.c_str(), data_str.size());
  }
  void LoadNodeTree(parsing::ParseNode* node);
  void LoadNodeTree(const dsl::Node& node);

  inline Rect rect() const { return m_rect; }
  // Sets the rect for this element in its parent.
  // The rect is relative to the parent element. The skin may expand outside
  // this rect to draw f.ex shadows.
  void set_rect(const Rect& rect);

  // Sets the position of this element in its parent.
  // The position is relative to the parent element.
  void set_position(const Point& pos) {
    set_rect(Rect(pos.x, pos.y, m_rect.w, m_rect.h));
  }

  // Sets size of this element.
  void set_size(int width, int height) {
    set_rect(Rect(m_rect.x, m_rect.y, width, height));
  }

  // Sizes the element to fit its parent, if it is in one.
  void FitToParent() {
    if (parent()) {
      set_rect(parent()->rect());
    }
  }

  // Invalidates should be called if the element need to be repainted, to make
  // sure the renderer repaints it and its children next frame.
  void Invalidate();

  // Call if something changes that might need other elements to update their
  // state.
  // F.ex if a action availability changes, some element might have to become
  // enabled/disabled.
  // Calling this will result in a later call to OnProcessStates().
  //
  // This is done automatically for all invoked events of type:
  // EventType::kClick, EventType::kLongClick, EventType::kChanged,
  // EventType::kKeyDown, EventType::kKeyUp
  void InvalidateStates();

  // Call if something changes that might cause any skin to change due to
  // different state or conditions. This is called automatically from
  // InvalidateStates(), when event EventType::kChanged is invoked, and in
  // various other situations.
  void InvalidateSkinStates();

  // Deletes the element with the possibility for some extended life during
  // animations.
  // If any element listener responds true to OnElementDying it will be kept as
  // a child and live until the animations are done, but the elements and all
  // its children are marked as dying. Dying elements get no input or focus.
  // If no element listener responded, it will be deleted immediately.
  void Die();

  // Returns true if this element or any of its parents is dying.
  bool is_dying() const {
    return m_packed.is_dying || (m_parent && m_parent->is_dying());
  }

  TBID& id() { return m_id; }
  // Sets the id reference for this elements. This id is 0 by default.
  // You can use this id to receive the element from GetElementById (or
  // preferable TBSafeGetByID to avoid dangerous casts).
  void set_id(const TBID& id);

  TBID& group_id() { return m_group_id; }
  // Sets the group id reference for this elements. This id is 0 by default.
  // All elements with the same group id under the same group root will be
  // automatically changed when one change its value.
  void set_group_id(const TBID& id) { m_group_id = id; }

  // Gets this element or any child element with a matching id and type, or
  // nullptr if none is found.
  template <typename T>
  T* GetElementById(const TBID& id) {
    return reinterpret_cast<T*>(GetElementByIdInternal(id, GetTypeId<T>()));
  }

  struct LookupPair {
    template <typename T>
    LookupPair(const el::TBID& id, T** out_ptr)
        : id(id),
          type_id(GetTypeId<T>()),
          out_ptr(reinterpret_cast<el::Element**>(out_ptr)) {}
    el::TBID id;
    el::util::tb_type_id_t type_id;
    el::Element** out_ptr;
  };
  // Gets all elements in the list by ID and stores them in the given pointers.
  // Returns false if one or more of the elements is not found.
  bool GetElementsById(std::vector<LookupPair> els);

  using State = SkinState;

  // Gets status of the given state(s).
  // Returns true if the given state combination is set.
  bool has_state(State state) const { return any(m_state & state); }

  // Enables or disables the given state(s).
  // The state affects which skin state is used when drawing.
  // Some states are set automatically on interaction. See computed_state().
  void set_state(State state, bool on);

  // Gets the element state.
  State state_raw() const { return m_state; }

  // Sets the element state.
  // Like set_state but setting the entire state as given, instead of toggling
  // individual states. See set_state for more info on states.
  void set_state_raw(State state);

  // Returns the current combined state for this element. It will also add some
  // automatic states, such as hovered (if the element is currently hovered), or
  // pressed etc.
  //
  // Automatic states:
  // SkinState::kPressed, SkinState::kHovered, SkinState::kFocused.
  //
  // RE SkinState::kFocused: may also be controlled by calling
  // set_auto_focus_state and the define EL_ALWAYS_SHOW_EDIT_FOCUS.
  State computed_state() const;

  // Sets whether the state SkinState::kFocused should be set automatically for
  // the focused element.
  // This value is set to true when moving focus by keyboard, and set to off
  // when clicking with the pointer.
  static void set_auto_focus_state(bool on);

  float opacity() const { return m_opacity; }
  // Sets opacity for this element and its children from 0.0 - 1.0.
  // If opacity is 0 (invisible), the element won't receive any input.
  void set_opacity(float opacity);

  Visibility visibility() const;
  // Sets visibility for this element and its children.
  // If visibility is not Visibility::kVisible, the element won't receive any
  // input.
  void set_visibility(Visibility vis);

  // Returns true if this element and all its ancestors are visible (has a
  // opacity > 0 and visibility Visibility::kVisible).
  bool computed_visibility() const;

  // Returns false if this element or any of its parents are disabled (has state
  // SkinState::kDisabled).
  bool is_enabled() const;
  // Sets whether the element is enabled (default) or disabled.
  void set_enabled(bool value) { set_state(Element::State::kDisabled, !value); }

  // Removes the element from its parent, if it is currently in one.
  void RemoveFromParent();

  // Adds a child to this element.
  // The child element will automatically be deleted when this element is
  // deleted unless the child is removed again with RemoveChild.
  void AddChild(Element* child, ElementZ z = ElementZ::kTop,
                InvokeInfo info = InvokeInfo::kNormal);

  // Adds a child to this element.
  // See AddChild for adding a child to the top or bottom.
  // This takes a relative Z and inserts the child before or after the given
  // reference element.
  void AddChildRelative(Element* child, ElementZRel z, Element* reference,
                        InvokeInfo info = InvokeInfo::kNormal);

  // Removes child from this element without deleting it.
  void RemoveChild(Element* child, InvokeInfo info = InvokeInfo::kNormal);

  // Removes child from this element and deletes it.
  void DeleteChild(Element* child, InvokeInfo info = InvokeInfo::kNormal);

  // Deletes a child and replaces it with the given new child in the same
  // location.
  void ReplaceChild(Element* old_child, Element* new_child,
                    InvokeInfo info = InvokeInfo::kNormal);

  // Removes and deletes all children in this element.
  // NOTE: This won't invoke Die so there's no chance for elements to survive or
  // animate. They will be instantly removed and deleted.
  void DeleteAllChildren();

  // Sets the z-order of this element related to its siblings.
  // When a element is added with AddChild, it will be placed at the top in the
  // parent (Above previously added element). SetZ can be used to change the
  // order.
  void set_z(ElementZ z);

  ElementZ z_inflate() const { return (ElementZ)m_packed.inflate_child_z; }
  // Sets the z-order in which children are added during resource loading.
  void set_z_inflate(ElementZ z) {
    m_packed.inflate_child_z = static_cast<int>(z) ? 1 : 0;
  }

  Gravity gravity() const { return m_gravity; }
  // Sets the element gravity (any combination of Gravity).
  // For child elements in a layout, the gravity affects how the layout is done
  // depending on the layout settings.
  // For child elements in a non-layout element, it will do some basic
  // resizing/moving:
  // - left && right: Element resize horizontally when parent resize.
  // - !left && right: Element follows the right edge when parent resize.
  // - top && bottom: Element resize vertically when parent resize.
  // - !top && bottom: Element follows the bottom edge when parent resize.
  void set_gravity(Gravity g);

  // Returns the current skin background, as set by SetSkinBg.
  TBID background_skin() const { return m_skin_bg; }

  // Sets the skin background for this element and call OnSkinChanged if it
  // changed.
  // The skin background is used for calculating padding, preferred size etc. if
  // the element doesn't have any preferences itself.
  //
  // The skin will be painted according to the current element state
  // (SkinState).
  // If there is no special skin state for SkinState::kFocused, it will paint
  // the skin element called "generic_focus" (if it exist) after painting all
  // element children.
  //
  // It's possible to omit the OnSkinChanged callback using
  // InvokeInfo::kNoCallbacks.
  void set_background_skin(const TBID& skin_bg,
                           InvokeInfo info = InvokeInfo::kNormal);

  // Returns the skin background element, or nullptr.
  SkinElement* background_skin_element();

  bool is_group_root() const { return m_packed.is_group_root; }
  // Sets if this element is a group root.
  // Grouped elements (such as RadioButton) will toggle all other elements with
  // the same group_id under the nearest parent group root.
  // Form is a group root by default.
  void set_group_root(bool group_root) { m_packed.is_group_root = group_root; }

  bool is_focusable() const { return m_packed.is_focusable; }
  // Sets if this element should be able to receive focus or not.
  void set_focusable(bool focusable) { m_packed.is_focusable = focusable; }

  bool is_click_by_key() const { return m_packed.click_by_key; }
  // Sets if this element should emulate a click when it's focused and pressing
  // enter or space.
  void set_click_by_key(bool click_by_key) {
    m_packed.click_by_key = click_by_key;
  }

  bool is_long_clickable() const { return m_packed.want_long_click; }
  // Sets if this element should generate long-click event (or context menu
  // event if nothing handles the long click event). The default is false.
  void set_long_clickable(bool want_long_click) {
    m_packed.want_long_click = want_long_click;
  }

  bool is_ignoring_input() const { return m_packed.ignore_input; }
  // Sets if this element should ignore input, as if it didn't exist.
  void set_ignoring_input(bool ignore_input) {
    m_packed.ignore_input = ignore_input;
  }

  // Gets if this element wants interaction depending on various states.
  // Cares about zero opacity, visibility, flag set by SetIgnoreInput, disabled
  // state, and if the element is currently dying.
  bool is_interactable() const;

  bool is_focused() const { return focused_element == this; }
  // Sets this element to be the focused element. It will be the one receiving
  // keyboard input.
  // Elements can be focused only after enabling it (see set_focusable(true)).
  // Invisible or disabled elements can not be focused.
  // If SetFocus is called on a element in a inactive form, it will succeed
  // (return true), but it won't actually become focused until that form is
  // activated (see Form::set_last_focus).
  // Returns true if successfully focused, or if set as last focus in its
  // form.
  bool set_focus(FocusReason reason, InvokeInfo info = InvokeInfo::kNormal);

  // Calls SetFocus on all children and their children, until a element is found
  // that accepts it.
  // Returns true if some child was successfully focused.
  bool set_focus_recursive(FocusReason reason);

  // Moves focus from the currently focused element to another focusable
  // element.
  // It will search for a focusable element in the same Form (or top root if
  // there is no form) forward or backwards in the element order.
  bool MoveFocus(bool forward);

  // Returns the child element that contains the coordinate or nullptr if no one
  // does.
  // If include_children is true, the search will recurse into the childrens
  // children.
  Element* GetElementAt(int x, int y, bool include_children) const;

  // Gets the child at the given index, or nullptr if there was no child at that
  // index.
  // NOTE: avoid calling this in loops since it does iteration. Consider
  // iterating the elements directly instead!
  Element* GetChildFromIndex(int index) const;

  // Gets the child index of the given element (that must be a child of this
  // element).
  // NOTE: avoid calling this in loops since it does iteration. Consider
  // iterating the elements directly instead!
  int GetIndexFromChild(Element* child) const;

  // Gets the text of a child element with the given id, or an empty string if
  // there was no element with that id.
  std::string GetTextById(const TBID& id);

  // Gets the value of a child element with the given id, or 0 if there was no
  // element with that id.
  int GetValueById(const TBID& id);

  Element* GetNextDeep(const Element* bounding_ancestor = nullptr) const;
  Element* GetPrevDeep() const;
  Element* GetLastLeaf() const;
  inline Element* first_child() const { return m_children.GetFirst(); }
  inline Element* last_child() const { return m_children.GetLast(); }
  util::IntrusiveList<Element>::Iterator begin() {
    return m_children.IterateForward();
  }
  util::IntrusiveList<Element>::Iterator rbegin() {
    return m_children.IterateBackward();
  }

  // Returns true if this element is the same or a ancestor of other_element.
  bool IsAncestorOf(Element* other_element) const;

  // Returns true if this element is the same as other_element or if
  // other_element events are going through this element (see
  // event_destination()).
  bool IsEventDestinationFor(Element* other_element) const;

  // Adds a listener to this element. It should be removed again with
  // RemoveListener before the element is deleted.
  void AddListener(ElementListener* listener);
  void RemoveListener(ElementListener* listener);
  bool HasListener(ElementListener* listener) const;

  void AddEventHandler(EventHandler* event_handler);
  void RemoveEventHandler(EventHandler* event_handler);

  // Callback for handling events.
  // Return true if the event is handled and should not continue to be handled
  // by any parent elements.
  // Subclasses overriding Element must call the default Element version.
  virtual bool OnEvent(const Event& ev);

  // Callback for doing anything that might be needed before paint.
  // F.ex Updating invalid layout, formatting text etc.
  virtual void OnProcess() {}

  // Callback for doing anything that might be needed before paint.
  // This is called after OnProcess has been called on this elements children.
  virtual void OnProcessAfterChildren() {}

  // Callback for doing state updates that depend on your application state.
  // F.ex setting the disabled state on a element which action is currently not
  // available. This callback is called for all elements before OnProcess if
  // something has called InvalidateStates().
  virtual void OnProcessStates() {}

  // Contains properties needed for painting a element.
  // Properties may be inherited from the parent element if not specified in the
  // skin.
  class PaintProps {
   public:
    PaintProps();

    // Text color as specified in the skin element, or inherited from parent.
    Color text_color;
  };

  // Callback for painting this element.
  // The skin is already painted and the opacity set to reflect this elements.
  // This is only called for elements with a opacity > 0.
  virtual void OnPaint(const PaintProps& paint_props) {}

  // Callback for painting child elements.
  // The default implementation is painting all children.
  virtual void OnPaintChildren(const PaintProps& paint_props);

  // Callback for when this element or any of its children have called
  // Invalidate().
  virtual void OnInvalid() {}

  // Called when the background skin changes by calling set_background_skin(),
  // or when the
  // skin has changed indirectly after a skin condition changes in a way that
  // may affect layout.
  // For indirect skin changes, OnSkinChanged is called before validation of
  // layouts is about to happen in InvokeProcess().
  virtual void OnSkinChanged() {}

  // Called when the font has changed.
  virtual void OnFontChanged() {}

  // Called when the focus has changed.
  virtual void OnFocusChanged(bool focused) {}

  // Called when the visibility has changed.
  // NOTE: this is not called when combined visibility change, so it may change
  // visibility because of ancestors without this being called.
  virtual void OnVisibilityChanged() {}

  // Called when the capture has changed.
  virtual void OnCaptureChanged(bool captured) {}

  // Called when a child element has been added to this element (before calling
  // OnAdded on child).
  virtual void OnChildAdded(Element* child) {}

  // Called when a child element is about to be removed from this element
  // (before calling OnRemove on child).
  virtual void OnChildRemove(Element* child) {}

  // Called when this element has been added to a parent (after calling
  // OnChildAdded on parent).
  virtual void OnAdded() {}

  // Called when a this element has been removed from its parent (after calling
  // OnChildRemove on parent).
  virtual void OnRemove() {}

  // Called when Die() is called on this element. Note: Not called for children
  // to the element Die() was invoked on even though they are also dying.
  virtual void OnDie() {}

  // Called when this element has been resized.
  // The default implementation move and resize all children according to their
  // gravity.
  virtual void OnResized(int old_w, int old_h);

  // Called when this element has been scrolled.
  virtual void OnScroll(int scroll_x, int scroll_y) {}

  // Called just after a child has been inflated into this element.
  // The default implementation will resize the child to it's preferred size and
  // position it according to the gravity. If you implement a layouting element,
  // you should override this to prevent doing unnecessary measuring.
  virtual void OnInflateChild(Element* child);

  // Called when this element is inflated from resources, before any children
  // has been inflated.
  // This will read generic element properties and add the element to the
  // hierarchy if it's not already added. If overridden, you must call the super
  // implementation.
  virtual void OnInflate(const parsing::InflateInfo& info);

  // Gets hit status tests if this element should be hit at the given
  // coordinate.
  // The default implementation checks the visibility, ignored input flag,
  // rectangle, and disabled status.
  virtual HitStatus GetHitStatus(int x, int y);

  // Gets if skin condition applies to this element.
  // This is called when a skin condition has the property SkinProperty::kCustom
  // (not a generic one known by skin and the default element condition
  // context).
  // This can be used to extend the skin conditions support with properties
  // specific to different elements.
  virtual bool GetCustomSkinCondition(
      const SkinCondition::ConditionInfo& info) {
    return false;
  }

  // Gets this element or a child element that should be root for other
  // children.
  // This is useful for elements having multiple children by default, to specify
  // which one that should get the children.
  virtual Element* content_root() { return this; }

  // Gets this element or a parent element that is the absolute root parent.
  Element* parent_root();

  // Gets the closest parent element that is a Form or nullptr if there is
  // none.
  // If this element is a form itself, this will be returned.
  elements::Form* parent_form();

  // Gets the parent element, or nullptr if this element is not added.
  inline Element* parent() const { return m_parent; }

  // Gets the element that should receive the events this element invoke. By
  // default the parent.
  virtual Element* event_destination() { return m_parent; }

  // Returns translation the children should have.
  // Any scrolling of child elements should be done with this method, by
  // returning the wanted translation.
  //
  // When implementing this, you must also implement ScrollTo and GetScrollInfo
  // so focus-scroll and panning will work automatically when dragging this or
  // any child element.
  //
  // NOTE: you can apply the translation on one element and implement those
  // methods on a parent, by returning this element from the parents
  // scroll_root().
  virtual void GetChildTranslation(int* x, int* y) const { *x = *y = 0; }

  // If this is a element that scroll children (see GetChildTranslation), it
  // should scroll to the coordinates x, y.
  // This must result in calling OnScroll if scrolling occured.
  virtual void ScrollTo(int x, int y) {}

  // Starts the Scroller for this element and scroll it to the given position.
  // Will cancel any on going smooth scroll operation.
  void ScrollToSmooth(int x, int y);

  // If this is a element that scroll children (see GetChildTranslation), it
  // will scroll by delta dx, dy relative to its current position.
  void ScrollBy(int dx, int dy);

  // Starts the Scroller for this element and scroll it by the given delta.
  // Consecutive calls will accumulate the scroll speed.
  void ScrollBySmooth(int dx, int dy);

  // Information about scrolling for a element at the time of calling
  // GetScrollInfo.
  class ScrollInfo {
   public:
    ScrollInfo() = default;
    bool CanScrollX() const { return max_x > min_x; }
    bool CanScrollY() const { return max_y > min_y; }
    bool CanScrollLeft() const { return x > min_x; }
    bool CanScrollRight() const { return x < max_x; }
    bool CanScrollUp() const { return y > min_y; }
    bool CanScrollDown() const { return y < max_y; }
    bool can_scroll() const { return CanScrollX() || CanScrollY(); }
    int min_x = 0, min_y = 0;  // Minimum x and y scroll position.
    int max_x = 0, max_y = 0;  // Maximum x and y scroll position.
    int x = 0, y = 0;          // Current x and y scroll position.
  };

  // If this is a element that scroll children (see GetChildTranslation), it
  // should return the current scroll information.
  virtual ScrollInfo scroll_info() { return ScrollInfo(); }

  // If this element is implementing ScrollTo and GetScrollInfo but the
  // corresponding GetChildTranslation is implemented on a child, you should
  // return that child from this method.
  virtual Element* scroll_root() { return this; }

  // Scrolls this element and/or any parent elements by the given delta.
  // dx and dy will be reduced by the amount that was successfully scrolled.
  void ScrollByRecursive(int* dx, int* dy);

  // Makes this element visible by calling ScrollIntoView on all parent
  // elements.
  void ScrollIntoViewRecursive();

  // If this is a element that scroll children (see GetChildTranslation), it
  // will scroll so that rect is visible. Rect is relative to this element.
  void ScrollIntoView(const Rect& rect);

  // Returns the Scroller set up for this element, creating it if needed.
  elements::parts::Scroller* scroller();

  virtual Axis axis() const { return Axis::kX; }
  // Sets along which axis the content should layout.
  virtual void set_axis(Axis axis) {}

  virtual int value() { return 0; }
  // Sets the value of this element.
  // Implemented by most elements that have a value.
  // NOTE: some elements also provide special setters with other types (such as
  // double).
  virtual void set_value(int value) {}

  // Returns the value in double precision.
  // It only makes sense to use this instead of value() on elements that
  // store the value as double.
  // F.ex ScrollBar, Slider.
  virtual double double_value() { return static_cast<double>(value()); }
  // Sets the value in double precision.
  // It only makes sense to use this instead of set_value() on elements that
  // store the value as double.
  // F.ex ScrollBar, Slider.
  virtual void set_double_value(double value) {
    set_value(static_cast<int>(value));
  }

  // Gets the text of this element. Implemented by most elements that have text.
  virtual std::string text() { return ""; }

  // Sets the text of this element. Implemented by most elements that have text.
  virtual void set_text(const char* text) {}
  void set_text(const std::string& text) { set_text(text.c_str()); }
  void set_text_format(const char* format, ...);

  // Gets the description string of this element. Used for tooltips.
  virtual const std::string& tooltip() { return m_tooltip_str; }
  // Sets the description string for this element. Used for tooltips.
  virtual void set_tooltip(const char* value) {
    m_tooltip_str = value ? value : "";
  }

  // Connects this element to a element value.
  // When this element invoke EventType::kChanged, it will automatically update
  // the connected element value, and any other elements that may be connected
  // to it.
  // On connection, the value of this element will be updated to the value of
  // the given ElementValue.
  void Connect(ElementValue* value) { m_connection.Connect(value, this); }

  // Disconnects, if this element is connected to a ElementValue.
  void Disconnect() { m_connection.Disconnect(); }

  // Gets the rectangle inside any padding, relative to this element.
  // This is the rectangle in which the content should be rendered.
  // This may be overridden to f.ex deduct space allocated by visible scrollbars
  // managed by this element. Anything that removes space from the content area.
  virtual Rect padding_rect();

  // Calculates the preferred content size for this element.
  // This is the size of the actual content. Don't care about padding or other
  // decoration.
  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  // Calculates the preferred size for this element.
  // This is the full size of the element, content + padding + eventual other
  // decoration (but not skin expansion).
  // This is the size that should be used for laying out a element.
  // The returned PreferredSize also contains minimal size and maximum size.
  virtual PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints);

  // Gets the PreferredSize for this element.
  // This returns cached data if valid, or calls OnCalculatePreferredSize if
  // needed.
  PreferredSize GetPreferredSize(const SizeConstraints& constraints);
  PreferredSize GetPreferredSize() {
    return GetPreferredSize(SizeConstraints());
  }

  // Type used for InvalidateLayout.
  enum class InvalidationMode {
    kTargetOnly,  // InvalidationMode should not be recursively called on
                  // parents.
    kRecursive,   // InvalidationMode should recursively be called on parents
                  // too.
  };

  // Invalidates layout for this element so it will be scheduled for relayout.
  // Any change to the size preferences for a element should call this to let
  // parent layout adjust to the change.
  //
  // Remarks for layout elements:
  // - When a layout element get this, it should mark its layout as invalid and
  //   do the layout later (in GetPreferredContentSize/GetPreferredSize are
  //   called). If a layout knows that no parents will be affected, it may stop
  //   recursion to parents to avoid unnecessary relayout.
  // - When setting the size of a layout element (typically from another layout
  //   element or from a OnResize), it should be called with
  //   InvalidationMode::kTargetOnly to avoid recursing back up to parents when
  //   already recursing down, to avoid unnecessary computation.
  virtual void InvalidateLayout(InvalidationMode il);

  // Gets layout params, or nullptr if not specified.
  // NOTE: the layout params has already been applied to the PreferredSize
  // returned from GetPreferredSize so you normally don't need to check these
  // params.
  const LayoutParams* layout_params() const { return m_layout_params.get(); }
  // Sets layout params. Calls InvalidateLayout.
  void set_layout_params(const LayoutParams& lp);

  // TODO(benvanik): move to a special RootElement type.

  // Invoke OnProcess and OnProcessAfterChildren on this element and its
  // children.
  void InvokeProcess();

  // Invoke OnProcessStates on all child elements, if state processing is needed
  // (InvalidateStates() has been called).
  void InvokeProcessStates(bool force_update = false);

  // Invoke paint on this element and all its children.
  void InvokePaint(const PaintProps& parent_paint_props);

  // Invoke OnFontChanged on this element and recursively on any children that
  // inherit the font.
  void InvokeFontChanged();

  // Invoke a event on this element.
  // This will first check with all registered ElementListener if the event
  // should be dispatched.
  // If the elements OnEvent returns false (event not handled), it will continue
  // traversing to GetEventDestination (by default the parent) until a element
  // handles the event.
  //
  // NOTE: when invoking event EventType::kChanged, this will update the value
  // of other elements connected to the same group.
  // NOTE: some event types will automatically invalidate states (see
  // InvalidateStates(), InvalidateSkinStates()).
  // NOTE: remember that this elements may be deleted after this call! So if you
  // really must do something after this call and are not sure what the event
  // will cause, use WeakElementPointer to detect self deletion.
  bool InvokeEvent(Event ev);

  bool InvokePointerDown(int x, int y, int click_count,
                         ModifierKeys modifierkeys, bool touch);
  bool InvokePointerUp(int x, int y, ModifierKeys modifierkeys, bool touch);
  void InvokePointerMove(int x, int y, ModifierKeys modifierkeys, bool touch);
  bool InvokeWheel(int x, int y, int delta_x, int delta_y,
                   ModifierKeys modifierkeys);

  // Invoke the EventType::kKeyDown and EventType::kKeyUp events on the
  // currently focused element.
  // This will also do some generic key handling, such as cycling focus on tab
  // etc.
  bool InvokeKey(int key, SpecialKey special_key, ModifierKeys modifierkeys,
                 bool down);

  // A element that receive a EventType::kPointerDown event will stay "captured"
  // until EventType::kPointerUp is received.
  // While captured all EventType::kPointerMove are sent to it. This method can
  // force release the capture, which may happen f.ex if the Element is removed
  // while captured.
  void ReleaseCapture();

  // Makes x and y (relative to this element) relative to the upper left corner
  // of the root element.
  void ConvertToRoot(int* x, int* y) const;

  // Makes x and y (relative to the upper left corner of the root element)
  // relative to this element.
  void ConvertFromRoot(int* x, int* y) const;

  // Gets the font description as set with SetFontDescription.
  // Use computed_font_description() to get the calculated font description
  // (inherit from parent element, etc).
  FontDescription font_description() const { return m_font_desc; }

  // Calculates the font description for this element.
  // If this element have unspecified font description, it will be inheritted
  // from parent. If no parent specify any font, the default font description
  // will be returned.
  FontDescription computed_font_description() const;

  // Sets the font description for this element and any children that inherit
  // the font.
  // Setting an unspecified FontDescription (no changes made since construction)
  // means it will be inherited from parent (the default).
  // Returns true and invokes OnFontChanged on all affected elements, if the
  // font was successfully set.
  // Returns false and keep the font onchanged if it no matching font exists or
  // fails creation.
  bool set_font_description(const FontDescription& font_desc);

  // Gets the FontFace for this element from the current font description
  // (calculated by GetCalculatedFontDescription).
  text::FontFace* computed_font() const;

 private:
  friend class ElementListener;
  Element* m_parent = nullptr;
  TBID m_id;                // ID for GetElementById and others.
  TBID m_group_id;          // ID for button groups (such as RadioButton)
  TBID m_skin_bg;           // ID for the background skin (0 for no skin).
  TBID m_skin_bg_expected;  // ID for the background skin after strong override,
                            // used to indirect skin changes because of
                            // condition changes.
  // The rectangle of this element, relative to the parent. See set_rect.
  Rect m_rect;
  util::IntrusiveList<Element> m_children;
  ElementValueConnection m_connection;
  util::IntrusiveList<ElementListener> m_listeners;
  util::IntrusiveList<EventHandler> event_handlers_;
  // Opacity 0-1. See SetOpacity.
  float m_opacity = 1.0f;
  // The element state (excluding any auto states).
  State m_state = State::kNone;
  Gravity m_gravity = Gravity::kDefault;
  FontDescription m_font_desc;
  PreferredSize m_cached_ps;    // Cached preferred size.
  SizeConstraints m_cached_sc;  // Cached size constraints.
  std::unique_ptr<LayoutParams> m_layout_params;
  std::unique_ptr<elements::parts::Scroller> m_scroller;
  std::unique_ptr<LongClickTimer> m_long_click_timer;
  std::string m_tooltip_str;
  union {
    struct {
      uint16_t is_group_root : 1;
      uint16_t is_focusable : 1;
      uint16_t click_by_key : 1;
      uint16_t has_key_pressed_state : 1;
      uint16_t ignore_input : 1;
      uint16_t is_dying : 1;
      uint16_t is_cached_ps_valid : 1;
      uint16_t no_automatic_hover_state : 1;
      uint16_t is_panning : 1;
      uint16_t want_long_click : 1;
      uint16_t visibility : 2;
      uint16_t inflate_child_z : 1;  // Should have enough bits to hold ElementZ
                                     // values.
    } m_packed;
    uint16_t m_packed_init = 0;
  };

 public:
  // This value is free to use for anything. It's not used by Element itself.
  // Initially Type::kNull.
  Value data;

#ifdef EL_RUNTIME_DEBUG_INFO
  uint64_t last_measure_time = 0;
  uint64_t last_layout_time = 0;
#endif  // EL_RUNTIME_DEBUG_INFO

  // The currently hovered element, or nullptr.
  static Element* hovered_element;
  // The currently captured element, or nullptr.
  static Element* captured_element;
  // The currently focused element, or nullptr.
  static Element* focused_element;
  // Pointer x position on down event, relative to the captured element.
  static int pointer_down_element_x;
  // Pointer y position on down event, relative to the captured element.
  static int pointer_down_element_y;
  // Pointer x position on last pointer event, relative to the captured element
  // (if any) or hovered element.
  static int pointer_move_element_x;
  // Pointer y position on last pointer event, relative to the captured element
  // (if any) or hovered element.
  static int pointer_move_element_y;
  // true if the pointer up event should not generate a click event.
  static bool cancel_click;
  // true if something has called InvalidateStates() and it still hasn't been
  // updated.
  static bool update_element_states;
  // true if something has called InvalidateStates() and skin still hasn't been
  // updated.
  static bool update_skin_states;
  // true if the focused state should be painted automatically.
  static bool show_focus_state;

  static void SetIdFromNode(TBID* id, parsing::ParseNode* node);

 private:
  // Returns this element or the nearest parent that is scrollable in the given
  // axis, or nullptr if there is none.
  Element* FindScrollableElement(bool scroll_x, bool scroll_y);
  elements::parts::Scroller* FindStartedScroller();
  elements::parts::Scroller* GetReadyScroller(bool scroll_x, bool scroll_y);
  Element* GetElementByIdInternal(const TBID& id,
                                  const util::tb_type_id_t type_id = nullptr);
  void InvokeSkinUpdatesInternal(bool force_update);
  void InvokeProcessInternal();
  static void SetHoveredElement(Element* element, bool touch);
  static void SetCapturedElement(Element* element);
  void HandlePanningOnMove(int x, int y);
  void StartLongClickTimer(bool touch);
  void StopLongClickTimer();
  friend class LongClickTimer;
  void MaybeInvokeLongClickOrContextMenu(bool touch);
  // Returns the opacity for this element multiplied with its skin opacity and
  // state opacity.
  float CalculateOpacityInternal(State state, SkinElement* skin_element) const;
};

// Gets this element or any child element with a matching id, or nullptr if
// none is found.
template <>
inline Element* Element::GetElementById<Element>(const TBID& id) {
  return GetElementByIdInternal(id);
}

// Check if a condition is true for a element when painting a skin.
class ElementSkinConditionContext : public SkinConditionContext {
 public:
  explicit ElementSkinConditionContext(Element* element) : m_element(element) {}
  bool GetCondition(SkinTarget target,
                    const SkinCondition::ConditionInfo& info) const override;

 private:
  bool GetCondition(Element* element,
                    const SkinCondition::ConditionInfo& info) const;
  Element* m_element = nullptr;
};

namespace dsl {

using el::Align;
using el::Axis;
using ElementState = el::Element::State;
using el::Gravity;
using el::Rect;
using el::TextAlign;
using el::Visibility;

template <typename T>
struct ElementNode : public Node {
  using R = T;
  ElementNode(const char* name,
              std::vector<std::pair<const char*, const char*>> properties = {},
              std::vector<Node> children = {})
      : Node(name, std::move(properties), std::move(children)) {}

  // TBID
  R& id(Id value) {
    value.set(this, "id");
    return *reinterpret_cast<R*>(this);
  }
  // TBID
  R& group_id(Id value) {
    value.set(this, "group-id");
    return *reinterpret_cast<R*>(this);
  }
  // TBID
  R& skin(Id value) {
    value.set(this, "skin");
    return *reinterpret_cast<R*>(this);
  }
  // Number
  R& data(int32_t value) {
    set("data", value);
    return *reinterpret_cast<R*>(this);
  }
  R& data(float value) {
    set("data", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_group_root(bool value) {
    set("is-group-root", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_focusable(bool value) {
    set("is-focusable", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_long_clickable(bool value) {
    set("want-long-click", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& ignore_input(bool value) {
    set("ignore-input", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& opacity(float value) {
    set("opacity", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& connection(std::string value) {
    set("connection", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& axis(Axis value) {
    set("axis", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
  //
  R& gravity(Gravity value) {
    set("gravity", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
  //
  R& visibility(Visibility value) {
    set("visibility", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
  //
  R& state(ElementState value) {
    set("state", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_enabled(bool value) {
    set("state",
        to_string(value ? ElementState::kNone : ElementState::kDisabled));
    return *reinterpret_cast<R*>(this);
  }
  // Rect
  R& rect(Rect value) {
    set("rect", std::move(value));
    return *reinterpret_cast<R*>(this);
  }
  //
  R& width(Dimension value) {
    set("lp>width", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& min_width(Dimension value) {
    set("lp>min-width", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& max_width(Dimension value) {
    set("lp>max-width", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& preferred_width(Dimension value) {
    set("lp>pref-width", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& height(Dimension value) {
    set("lp>height", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& min_height(Dimension value) {
    set("lp>min-height", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& max_height(Dimension value) {
    set("lp>max-height", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& preferred_height(Dimension value) {
    set("lp>pref-height", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& tooltip(std::string value) {
    set("tooltip", value);
    return *reinterpret_cast<R*>(this);
  }
  // The Element will be focused automatically the first time its Form is
  // activated.
  R& autofocus(bool value) {
    set("autofocus", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& font(const char* name, int32_t size_px) {
    set("font>name", name);
    set("font>size", size_px);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& font_name(std::string value) {
    set("font>name", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& font_size(Dimension value) {
    set("font>size", value);
    return *reinterpret_cast<R*>(this);
  }

  R& clone(Node node) {
    parse_node_->Add(CloneNode(node).parse_node());
    return *reinterpret_cast<R*>(this);
  }
  R& child(Node node) {
    parse_node_->Add(node.parse_node());
    return *reinterpret_cast<R*>(this);
  }
  R& children() { return *reinterpret_cast<R*>(this); }
  template <typename... C>
  R& children(Node child_node, C... other_children) {
    child(child_node);
    children(other_children...);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl

}  // namespace el

#endif  // EL_ELEMENT_H_
