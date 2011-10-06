// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/screen.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/logging.h"
#include "ui/aura/desktop.h"
#include "ui/aura/window.h"
#include "ui/gfx/native_widget_types.h"

namespace {

gfx::Rect GetMonitorAreaOrWorkAreaNearestPoint(const gfx::Point& point,
                                               bool work_area) {
  // TODO(oshima): Take point/work_area into account. Support multiple monitors.
  return gfx::Rect(aura::Desktop::GetInstance()->GetSize());
}

}  // namespace

namespace gfx {

// static
gfx::Point Screen::GetCursorScreenPoint() {
#if defined(OS_WIN)
  POINT pt;
  GetCursorPos(&pt);
  return gfx::Point(pt);
#endif
  NOTIMPLEMENTED();
  return gfx::Point();
}

// static
gfx::Rect Screen::GetMonitorWorkAreaNearestWindow(gfx::NativeWindow window) {
  gfx::Rect bounds = GetMonitorAreaNearestWindow(window);
  // Emulate that a work area can be smaller than its monitor.
  bounds.Inset(10, 10, 10, 10);
  return bounds;
}

// static
gfx::Rect Screen::GetMonitorAreaNearestWindow(gfx::NativeWindow window) {
  // TODO(oshima): Take point/work_area into account. Support multiple monitors.
  return gfx::Rect(aura::Desktop::GetInstance()->GetSize());
}

// static
gfx::Rect Screen::GetMonitorWorkAreaNearestPoint(const gfx::Point& point) {
  return GetMonitorAreaOrWorkAreaNearestPoint(point, true);
}

// static
gfx::Rect Screen::GetMonitorAreaNearestPoint(const gfx::Point& point) {
  return GetMonitorAreaOrWorkAreaNearestPoint(point, false);
}

gfx::NativeWindow Screen::GetWindowAtCursorScreenPoint() {
  NOTIMPLEMENTED();
  return NULL;
}

}  // namespace gfx
