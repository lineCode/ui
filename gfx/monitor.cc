// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/monitor.h"

#include "ui/gfx/insets.h"
#include "base/stringprintf.h"

namespace gfx {

Monitor::Monitor() : id_(-1), device_scale_factor_(1.0) {
}

Monitor::Monitor(int id) : id_(id), device_scale_factor_(1.0) {
}

Monitor::Monitor(int id, const gfx::Rect& bounds)
    : id_(id),
      bounds_(bounds),
      work_area_(bounds),
      device_scale_factor_(1.0) {
#if defined(USE_ASH)
  SetScaleAndBounds(device_scale_factor_, bounds);
#endif
}

Monitor::~Monitor() {
}

void Monitor::SetScaleAndBounds(
    float device_scale_factor,
    const gfx::Rect& bounds_in_pixel) {
  Insets insets = bounds_.InsetsFrom(work_area_);
  device_scale_factor_ = device_scale_factor;
#if defined(USE_ASH)
  bounds_in_pixel_ = bounds_in_pixel;
#endif
  // TODO(oshima): For m19, work area/monitor bounds that chrome/webapps sees
  // has (0, 0) origin because it's simpler and enough. Fix this when
  // real multi monitor support is implemented.
#if defined(ENABLE_DIP)
  bounds_ = gfx::Rect(
      bounds_in_pixel.size().Scale(1.0f / device_scale_factor_));
#else
  bounds_ = gfx::Rect(bounds_in_pixel.size());
#endif
  UpdateWorkAreaFromInsets(insets);
}

void Monitor::SetSize(const gfx::Size& size_in_pixel) {
  SetScaleAndBounds(
      device_scale_factor_,
#if defined(USE_ASH)
      gfx::Rect(bounds_in_pixel_.origin(), size_in_pixel));
#else
      gfx::Rect(bounds_.origin(), size_in_pixel));
#endif
}

void Monitor::UpdateWorkAreaFromInsets(const gfx::Insets& insets) {
  work_area_ = bounds_;
  work_area_.Inset(insets);
}

std::string Monitor::ToString() const {
  return base::StringPrintf("Monitor[%d] bounds=%s, workarea=%s, scale=%f",
                            id_,
                            bounds_.ToString().c_str(),
                            work_area_.ToString().c_str(),
                            device_scale_factor_);
}

}  // namespace gfx
