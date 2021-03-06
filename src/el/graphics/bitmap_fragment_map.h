/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_BITMAP_FRAGMENT_MAP_H_
#define EL_GRAPHICS_BITMAP_FRAGMENT_MAP_H_

#include <memory>
#include <string>
#include <vector>

#include "el/graphics/bitmap_fragment.h"

namespace el {
namespace graphics {

class BitmapFragmentManager;

// Used to pack multiple bitmaps into a single Bitmap.
// When initialized (in a size suitable for a Bitmap) is also creates a
// software buffer that will make up the Bitmap when all fragments have been
// added.
class BitmapFragmentMap {
 public:
  BitmapFragmentMap();
  ~BitmapFragmentMap();

  // Initializes the map with the given size.
  // The size should be a power of two since it will be used to create a
  // Bitmap (texture memory).
  bool Init(int bitmap_w, int bitmap_h);

  // Creates a new fragment with the given size and data in this map.
  // Returns nullptr if there is not enough room in this map or on any other
  // fail.
  std::unique_ptr<BitmapFragment> CreateNewFragment(int frag_w, int frag_h,
                                                    int data_stride,
                                                    uint32_t* frag_data,
                                                    bool add_border);

  // Frees up the space used by the given fragment, so that other fragments can
  // take its place.
  void FreeFragmentSpace(BitmapFragment* frag);

  // Returns the bitmap for this map.
  // By default, the bitmap is validated if needed before returning (See
  // Validate).
  Bitmap* GetBitmap(Validate validate_type = Validate::kAlways);

 private:
  friend class BitmapFragmentManager;
  bool ValidateBitmap();
  void DeleteBitmap();
  void CopyData(BitmapFragment* frag, int data_stride, uint32_t* frag_data,
                int border);

  std::vector<std::unique_ptr<BitmapFragmentSpaceAllocator>> m_rows;
  int m_bitmap_w = 0;
  int m_bitmap_h = 0;
  uint32_t* m_bitmap_data = nullptr;
  std::unique_ptr<Bitmap> m_bitmap;
  bool m_need_update = false;
  int m_allocated_pixels = 0;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_BITMAP_FRAGMENT_MAP_H_
