// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/event_filter.h"

namespace aura {

bool EventFilter::PreHandleKeyEvent(Window* target, ui::KeyEvent* event) {
  return false;
}

bool EventFilter::PreHandleMouseEvent(Window* target, ui::MouseEvent* event) {
  return false;
}

ui::TouchStatus EventFilter::PreHandleTouchEvent(Window* target,
                                                 ui::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus EventFilter::PreHandleGestureEvent(Window* target,
                                                     ui::GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;
}

bool EventFilter::PostHandleKeyEvent(Window* target, ui::KeyEvent* event) {
  return false;
}

bool EventFilter::PostHandleMouseEvent(Window* target, ui::MouseEvent* event) {
  return false;
}

ui::TouchStatus EventFilter::PostHandleTouchEvent(Window* target,
                                                  ui::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus EventFilter::PostHandleGestureEvent(Window* target,
                                                      ui::GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;
}

}  // namespace aura
