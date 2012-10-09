// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SIZE_BASE_H_
#define UI_GFX_SIZE_BASE_H_

#include <string>

#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "ui/base/ui_export.h"

namespace gfx {

// A size has width and height values.
template<typename Class, typename Type>
class UI_EXPORT SizeBase {
 public:
  Type width() const { return width_; }
  Type height() const { return height_; }

  Type GetArea() const { return width_ * height_; }

  void SetSize(Type width, Type height) {
    set_width(width);
    set_height(height);
  }

  void Enlarge(Type width, Type height) {
    set_width(width_ + width);
    set_height(height_ + height);
  }

  void set_width(Type width) { width_ = width; }
  void set_height(Type height) { height_ = height; }

  bool operator==(const Class& s) const {
    return width_ == s.width_ && height_ == s.height_;
  }

  bool operator!=(const Class& s) const {
    return !(*this == s);
  }

  bool IsEmpty() const {
    return (width_ <= 0) || (height_ <= 0);
  }

  void ClampToNonNegative() {
    if (width_ < 0)
      width_ = 0;
    if (height_ < 0)
      height_ = 0;
  }

 protected:
  SizeBase(Type width, Type height)
      : width_(width),
        height_(height) {}

  // Destructor is intentionally made non virtual and protected.
  // Do not make this public.
  ~SizeBase() {}

 private:
  Type width_;
  Type height_;
};

}  // namespace gfx

#endif  // UI_GFX_SIZE_BASE_H_
