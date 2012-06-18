// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/test_window_delegate.h"

#include "ui/aura/event.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/path.h"
#include "ui/gfx/skia_util.h"

namespace aura {
namespace test {

////////////////////////////////////////////////////////////////////////////////
// TestWindowDelegate

TestWindowDelegate::TestWindowDelegate() : window_component_(HTCLIENT) {
}

TestWindowDelegate::~TestWindowDelegate() {
}

gfx::Size TestWindowDelegate::GetMinimumSize() const {
  return gfx::Size();
}

void TestWindowDelegate::OnBoundsChanged(const gfx::Rect& old_bounds,
                                         const gfx::Rect& new_bounds) {
}

void TestWindowDelegate::OnFocus(Window* old_focused_window) {
}

void TestWindowDelegate::OnBlur() {
}

bool TestWindowDelegate::OnKeyEvent(KeyEvent* event) {
  return false;
}

gfx::NativeCursor TestWindowDelegate::GetCursor(const gfx::Point& point) {
  return gfx::kNullCursor;
}

int TestWindowDelegate::GetNonClientComponent(const gfx::Point& point) const {
  return window_component_;
}

bool TestWindowDelegate::ShouldDescendIntoChildForEventHandling(
      Window* child,
      const gfx::Point& location) {
  return true;
}

bool TestWindowDelegate::OnMouseEvent(MouseEvent* event) {
  return false;
}

ui::TouchStatus TestWindowDelegate::OnTouchEvent(TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus TestWindowDelegate::OnGestureEvent(GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;
}

bool TestWindowDelegate::CanFocus() {
  return true;
}

void TestWindowDelegate::OnCaptureLost() {
}

void TestWindowDelegate::OnPaint(gfx::Canvas* canvas) {
}

void TestWindowDelegate::OnDeviceScaleFactorChanged(
    float device_scale_factor) {
}

void TestWindowDelegate::OnWindowDestroying() {
}

void TestWindowDelegate::OnWindowDestroyed() {
}

void TestWindowDelegate::OnWindowVisibilityChanged(bool visible) {
}

bool TestWindowDelegate::HasHitTestMask() const {
  return false;
}

void TestWindowDelegate::GetHitTestMask(gfx::Path* mask) const {
}

////////////////////////////////////////////////////////////////////////////////
// ColorTestWindowDelegate

ColorTestWindowDelegate::ColorTestWindowDelegate(SkColor color)
    : color_(color),
      last_key_code_(ui::VKEY_UNKNOWN) {
}
ColorTestWindowDelegate::~ColorTestWindowDelegate() {
}

bool ColorTestWindowDelegate::OnKeyEvent(KeyEvent* event) {
  last_key_code_ = event->key_code();
  return true;
}
void ColorTestWindowDelegate::OnWindowDestroyed() {
  delete this;
}
void ColorTestWindowDelegate::OnPaint(gfx::Canvas* canvas) {
  canvas->DrawColor(color_, SkXfermode::kSrc_Mode);
}

////////////////////////////////////////////////////////////////////////////////
// MaskedWindowDelegate

MaskedWindowDelegate::MaskedWindowDelegate(const gfx::Rect mask_rect)
    : mask_rect_(mask_rect) {
}

bool MaskedWindowDelegate::HasHitTestMask() const {
  return true;
}

void MaskedWindowDelegate::GetHitTestMask(gfx::Path* mask) const {
  mask->addRect(RectToSkRect(mask_rect_));
}

}  // namespace test
}  // namespace aura
