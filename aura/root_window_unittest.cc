// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/root_window.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
#include "ui/aura/event.h"
#include "ui/aura/event_filter.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

namespace aura {
namespace {

// A delegate that always returns a non-client component for hit tests.
class NonClientDelegate : public test::TestWindowDelegate {
 public:
  NonClientDelegate()
      : non_client_count_(0),
        mouse_event_count_(0),
        mouse_event_flags_(0x0) {
  }
  virtual ~NonClientDelegate() {}

  int non_client_count() const { return non_client_count_; }
  gfx::Point non_client_location() const { return non_client_location_; }
  int mouse_event_count() const { return mouse_event_count_; }
  gfx::Point mouse_event_location() const { return mouse_event_location_; }
  int mouse_event_flags() const { return mouse_event_flags_; }

  virtual int GetNonClientComponent(const gfx::Point& location) const OVERRIDE {
    NonClientDelegate* self = const_cast<NonClientDelegate*>(this);
    self->non_client_count_++;
    self->non_client_location_ = location;
    return HTTOPLEFT;
  }
  virtual bool OnMouseEvent(MouseEvent* event) OVERRIDE {
    mouse_event_count_++;
    mouse_event_location_ = event->location();
    mouse_event_flags_ = event->flags();
    return true;
  }

 private:
  int non_client_count_;
  gfx::Point non_client_location_;
  int mouse_event_count_;
  gfx::Point mouse_event_location_;
  int mouse_event_flags_;

  DISALLOW_COPY_AND_ASSIGN(NonClientDelegate);
};

// A simple EventFilter that keeps track of the number of key events that it's
// seen.
class TestEventFilter : public EventFilter {
 public:
  TestEventFilter() : num_key_events_(0) {}
  virtual ~TestEventFilter() {}

  int num_key_events() const { return num_key_events_; }

  // EventFilter overrides:
  virtual bool PreHandleKeyEvent(Window* target, KeyEvent* event) OVERRIDE {
    num_key_events_++;
    return true;
  }
  virtual bool PreHandleMouseEvent(Window* target, MouseEvent* event) OVERRIDE {
    return false;
  }
  virtual ui::TouchStatus PreHandleTouchEvent(
      Window* target, TouchEvent* event) OVERRIDE {
    return ui::TOUCH_STATUS_UNKNOWN;
  }
  virtual ui::GestureStatus PreHandleGestureEvent(
      Window* target, GestureEvent* event) OVERRIDE {
    return ui::GESTURE_STATUS_UNKNOWN;
  }

 private:
  // How many key events have been received?
  int num_key_events_;

  DISALLOW_COPY_AND_ASSIGN(TestEventFilter);
};

}  // namespace

typedef test::AuraTestBase RootWindowTest;

TEST_F(RootWindowTest, DispatchMouseEvent) {
  // Create two non-overlapping windows so we don't have to worry about which
  // is on top.
  scoped_ptr<NonClientDelegate> delegate1(new NonClientDelegate());
  scoped_ptr<NonClientDelegate> delegate2(new NonClientDelegate());
  const int kWindowWidth = 123;
  const int kWindowHeight = 45;
  gfx::Rect bounds1(100, 200, kWindowWidth, kWindowHeight);
  gfx::Rect bounds2(300, 400, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window1(CreateTestWindowWithDelegate(
      delegate1.get(), -1234, bounds1, NULL));
  scoped_ptr<aura::Window> window2(CreateTestWindowWithDelegate(
      delegate2.get(), -5678, bounds2, NULL));

  // Send a mouse event to window1.
  gfx::Point point(101, 201);
  MouseEvent event1(
      ui::ET_MOUSE_PRESSED, point, point, ui::EF_LEFT_MOUSE_BUTTON);
  root_window()->DispatchMouseEvent(&event1);

  // Event was tested for non-client area for the target window.
  EXPECT_EQ(1, delegate1->non_client_count());
  EXPECT_EQ(0, delegate2->non_client_count());
  // The non-client component test was in local coordinates.
  EXPECT_EQ(gfx::Point(1, 1), delegate1->non_client_location());
  // Mouse event was received by target window.
  EXPECT_EQ(1, delegate1->mouse_event_count());
  EXPECT_EQ(0, delegate2->mouse_event_count());
  // Event was in local coordinates.
  EXPECT_EQ(gfx::Point(1, 1), delegate1->mouse_event_location());
  // Non-client flag was set.
  EXPECT_TRUE(delegate1->mouse_event_flags() & ui::EF_IS_NON_CLIENT);
}

// Check that we correctly track the state of the mouse buttons in response to
// button press and release events.
TEST_F(RootWindowTest, MouseButtonState) {
  EXPECT_FALSE(Env::GetInstance()->is_mouse_button_down());

  gfx::Point location;
  scoped_ptr<MouseEvent> event;

  // Press the left button.
  event.reset(new MouseEvent(
      ui::ET_MOUSE_PRESSED,
      location,
      location,
      ui::EF_LEFT_MOUSE_BUTTON));
  root_window()->DispatchMouseEvent(event.get());
  EXPECT_TRUE(Env::GetInstance()->is_mouse_button_down());

  // Additionally press the right.
  event.reset(new MouseEvent(
      ui::ET_MOUSE_PRESSED,
      location,
      location,
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON));
  root_window()->DispatchMouseEvent(event.get());
  EXPECT_TRUE(Env::GetInstance()->is_mouse_button_down());

  // Release the left button.
  event.reset(new MouseEvent(
      ui::ET_MOUSE_RELEASED,
      location,
      location,
      ui::EF_RIGHT_MOUSE_BUTTON));
  root_window()->DispatchMouseEvent(event.get());
  EXPECT_TRUE(Env::GetInstance()->is_mouse_button_down());

  // Release the right button.  We should ignore the Shift-is-down flag.
  event.reset(new MouseEvent(
      ui::ET_MOUSE_RELEASED,
      location,
      location,
      ui::EF_SHIFT_DOWN));
  root_window()->DispatchMouseEvent(event.get());
  EXPECT_FALSE(Env::GetInstance()->is_mouse_button_down());

  // Press the middle button.
  event.reset(new MouseEvent(
      ui::ET_MOUSE_PRESSED,
      location,
      location,
      ui::EF_MIDDLE_MOUSE_BUTTON));
  root_window()->DispatchMouseEvent(event.get());
  EXPECT_TRUE(Env::GetInstance()->is_mouse_button_down());
}

TEST_F(RootWindowTest, TranslatedEvent) {
  scoped_ptr<Window> w1(test::CreateTestWindowWithDelegate(NULL, 1,
      gfx::Rect(50, 50, 100, 100), NULL));

  gfx::Point origin(100, 100);
  MouseEvent root(ui::ET_MOUSE_PRESSED, origin, origin, 0);

  EXPECT_EQ("100,100", root.location().ToString());
  EXPECT_EQ("100,100", root.root_location().ToString());

  MouseEvent translated_event(
      root, root_window(), w1.get(),
      ui::ET_MOUSE_ENTERED, root.flags());
  EXPECT_EQ("50,50", translated_event.location().ToString());
  EXPECT_EQ("100,100", translated_event.root_location().ToString());
}

TEST_F(RootWindowTest, IgnoreUnknownKeys) {
  TestEventFilter* filter = new TestEventFilter;
  root_window()->SetEventFilter(filter);  // passes ownership

  KeyEvent unknown_event(ui::ET_KEY_PRESSED, ui::VKEY_UNKNOWN, 0);
  EXPECT_FALSE(root_window()->DispatchKeyEvent(&unknown_event));
  EXPECT_EQ(0, filter->num_key_events());

  KeyEvent known_event(ui::ET_KEY_PRESSED, ui::VKEY_A, 0);
  EXPECT_TRUE(root_window()->DispatchKeyEvent(&known_event));
  EXPECT_EQ(1, filter->num_key_events());
}

}  // namespace aura
