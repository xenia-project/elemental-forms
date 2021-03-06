/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ANIMATION_MANAGER_H_
#define EL_ANIMATION_MANAGER_H_

#include <cstdint>

#include "el/animation.h"
#include "el/util/intrusive_list.h"
#include "el/util/object.h"

namespace el {

// System class that manages all animated object.
class AnimationManager {
 private:
  static util::IntrusiveList<Animation> animating_objects;
  static int block_animations_counter;

 public:
  // Updates all running animations.
  static void Update();

  // Returns true if there is running animations.
  static bool has_running_animations();

  static void StartAnimation(
      Animation* obj, AnimationCurve animation_curve = Animation::kDefaultCurve,
      uint64_t animation_duration = Animation::kDefaultDuration,
      AnimationTime animation_time = AnimationTime::kFirstUpdate);

  // Aborts the given animation.
  // If delete_animation is true, the animation will be deleted in this call
  // after running callbacks and listeners callbacks. In rare situations, you
  // might want to keep the animation around and delete it later (or start it
  // again).
  static void AbortAnimation(Animation* obj, bool delete_animation);

  // Aborts and deletes all animations.
  static void AbortAllAnimations();

  // Returns true if new animations are blocked.
  static bool is_animation_blocked();

  // Begins a period of blocking new animations. End the period with
  // EndBlockAnimations.
  // If StartAnimation is called during the blocked period, the animation object
  // will finish the next animation update as it completed normally.
  static void BeginBlockAnimations();

  // Ends a period of blocking new animations that was started with
  // BeginBlockAnimations.
  static void EndBlockAnimations();
};

// Blocks new animations during its lifetime.
// It's convenient to put on the stack to block new animations within a scope of
// code.
class AnimationBlocker {
 public:
  AnimationBlocker() { AnimationManager::BeginBlockAnimations(); }
  ~AnimationBlocker() { AnimationManager::EndBlockAnimations(); }
};

}  // namespace el

#endif  // EL_ANIMATION_MANAGER_H_
