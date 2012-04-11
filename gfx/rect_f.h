// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_RECT_F_H_
#define UI_GFX_RECT_F_H_
#pragma once

#include <string>

#include "ui/gfx/point_f.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/rect_base.h"
#include "ui/gfx/size_f.h"

#if !defined(ENABLE_DIP)
#error "This class should be used only when DIP feature is enabled"
#endif

namespace gfx {

class InsetsF;

// A floating version of gfx::Rect.
class UI_EXPORT RectF : public RectBase<RectF, PointF, SizeF, InsetsF, float> {
 public:
  RectF();
  RectF(float width, float height);
  RectF(float x, float y, float width, float height);
  explicit RectF(const gfx::SizeF& size);
  RectF(const gfx::PointF& origin, const gfx::SizeF& size);

  virtual ~RectF();

  Rect ToRect() const;

  std::string ToString() const;
};

}  // namespace gfx

#endif  // UI_GFX_RECT_H_
