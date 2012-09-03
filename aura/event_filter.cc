// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/event_filter.h"

#include "ui/aura/window.h"

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

ui::EventResult EventFilter::OnKeyEvent(ui::EventTarget* target,
                                        ui::KeyEvent* event) {
  return PreHandleKeyEvent(static_cast<Window*>(target), event) ?
      ui::ER_CONSUMED : ui::ER_UNHANDLED;
}

ui::EventResult EventFilter::OnMouseEvent(ui::EventTarget* target,
                                          ui::MouseEvent* event) {
  return PreHandleMouseEvent(static_cast<Window*>(target), event) ?
      ui::ER_CONSUMED : ui::ER_UNHANDLED;
}

ui::EventResult EventFilter::OnScrollEvent(ui::EventTarget* target,
                                           ui::ScrollEvent* event) {
  return ui::ER_UNHANDLED;
}

ui::TouchStatus EventFilter::OnTouchEvent(ui::EventTarget* target,
                                          ui::TouchEvent* event) {
  return PreHandleTouchEvent(static_cast<Window*>(target), event);
}

ui::EventResult EventFilter::OnGestureEvent(ui::EventTarget* target,
                                            ui::GestureEvent* event) {
  return PreHandleGestureEvent(static_cast<Window*>(target), event) ==
      ui::GESTURE_STATUS_CONSUMED ? ui::ER_CONSUMED : ui::ER_UNHANDLED;
}

}  // namespace aura
