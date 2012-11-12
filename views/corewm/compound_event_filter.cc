// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/corewm/compound_event_filter.h"

#include "base/hash_tables.h"
#include "ui/aura/client/activation_client.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/env.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_tracker.h"
#include "ui/base/events/event.h"
#include "ui/base/hit_test.h"

namespace views {
namespace corewm {

namespace {

aura::Window* FindFocusableWindowFor(aura::Window* window) {
  while (window && !window->CanFocus())
    window = window->parent();
  return window;
}

aura::Window* GetActiveWindow(aura::Window* window) {
  DCHECK(window->GetRootWindow());
  return aura::client::GetActivationClient(window->GetRootWindow())->
      GetActiveWindow();
}

bool ShouldHideCursorOnKeyEvent(const ui::KeyEvent& event) {
  // All alt and control key commands are ignored.
  if (event.IsAltDown() || event.IsControlDown())
    return false;

  static bool inited = false;
  static base::hash_set<int32> ignored_keys;
  if (!inited) {
    // Modifiers.
    ignored_keys.insert(ui::VKEY_SHIFT);
    ignored_keys.insert(ui::VKEY_CONTROL);
    ignored_keys.insert(ui::VKEY_MENU);

    // Search key == VKEY_LWIN.
    ignored_keys.insert(ui::VKEY_LWIN);

    // Function keys.
    for (int key = ui::VKEY_F1; key <= ui::VKEY_F24; ++key)
      ignored_keys.insert(key);

    // Media keys.
    for (int key = ui::VKEY_BROWSER_BACK; key <= ui::VKEY_MEDIA_LAUNCH_APP2;
         ++key) {
      ignored_keys.insert(key);
    }

#if defined(OS_POSIX)
    ignored_keys.insert(ui::VKEY_WLAN);
    ignored_keys.insert(ui::VKEY_POWER);
    ignored_keys.insert(ui::VKEY_BRIGHTNESS_DOWN);
    ignored_keys.insert(ui::VKEY_BRIGHTNESS_UP);
    ignored_keys.insert(ui::VKEY_KBD_BRIGHTNESS_DOWN);
    ignored_keys.insert(ui::VKEY_KBD_BRIGHTNESS_UP);
#endif

    inited = true;
  }

  if (ignored_keys.count(event.key_code()) > 0)
    return false;

  return true;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// CompoundEventFilter, public:

CompoundEventFilter::CompoundEventFilter() : cursor_hidden_by_filter_(false) {
}

CompoundEventFilter::~CompoundEventFilter() {
  // Additional filters are not owned by CompoundEventFilter and they
  // should all be removed when running here. |filters_| has
  // check_empty == true and will DCHECK failure if it is not empty.
}

// static
gfx::NativeCursor CompoundEventFilter::CursorForWindowComponent(
    int window_component) {
  switch (window_component) {
    case HTBOTTOM:
      return ui::kCursorSouthResize;
    case HTBOTTOMLEFT:
      return ui::kCursorSouthWestResize;
    case HTBOTTOMRIGHT:
      return ui::kCursorSouthEastResize;
    case HTLEFT:
      return ui::kCursorWestResize;
    case HTRIGHT:
      return ui::kCursorEastResize;
    case HTTOP:
      return ui::kCursorNorthResize;
    case HTTOPLEFT:
      return ui::kCursorNorthWestResize;
    case HTTOPRIGHT:
      return ui::kCursorNorthEastResize;
    default:
      return ui::kCursorNull;
  }
}

void CompoundEventFilter::AddFilter(aura::EventFilter* filter) {
  filters_.AddObserver(filter);
}

void CompoundEventFilter::RemoveFilter(aura::EventFilter* filter) {
  filters_.RemoveObserver(filter);
}

////////////////////////////////////////////////////////////////////////////////
// CompoundEventFilter, private:

void CompoundEventFilter::UpdateCursor(aura::Window* target,
                                       ui::MouseEvent* event) {
  // If drag and drop is in progress, let the drag drop client set the cursor
  // instead of setting the cursor here.
  aura::RootWindow* root_window = target->GetRootWindow();
  aura::client::DragDropClient* drag_drop_client =
      aura::client::GetDragDropClient(root_window);
  if (drag_drop_client && drag_drop_client->IsDragDropInProgress())
    return;

  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(root_window);
  if (cursor_client) {
    gfx::NativeCursor cursor = target->GetCursor(event->location());
    if (event->flags() & ui::EF_IS_NON_CLIENT) {
      int window_component =
          target->delegate()->GetNonClientComponent(event->location());
      cursor = CursorForWindowComponent(window_component);
    }

    cursor_client->SetCursor(cursor);
  }
}

ui::EventResult CompoundEventFilter::FilterKeyEvent(ui::KeyEvent* event) {
  int result = ui::ER_UNHANDLED;
  if (filters_.might_have_observers()) {
    ObserverListBase<aura::EventFilter>::Iterator it(filters_);
    EventFilter* filter;
    while (!(result & ui::ER_CONSUMED) && (filter = it.GetNext()) != NULL)
      result |= filter->OnKeyEvent(event);
  }
  return static_cast<ui::EventResult>(result);
}

ui::EventResult CompoundEventFilter::FilterMouseEvent(ui::MouseEvent* event) {
  int result = ui::ER_UNHANDLED;
  if (filters_.might_have_observers()) {
    ObserverListBase<aura::EventFilter>::Iterator it(filters_);
    EventFilter* filter;
    while (!(result & ui::ER_CONSUMED) && (filter = it.GetNext()) != NULL)
      result |= filter->OnMouseEvent(event);
  }
  return static_cast<ui::EventResult>(result);
}

ui::EventResult CompoundEventFilter::FilterTouchEvent(ui::TouchEvent* event) {
  int result = ui::ER_UNHANDLED;
  if (filters_.might_have_observers()) {
    ObserverListBase<aura::EventFilter>::Iterator it(filters_);
    EventFilter* filter;
    while (!(result & ui::ER_CONSUMED) && (filter = it.GetNext()) != NULL) {
      result |= filter->OnTouchEvent(event);
    }
  }
  return static_cast<ui::EventResult>(result);
}

void CompoundEventFilter::SetCursorVisibilityOnEvent(aura::Window* target,
                                                     ui::Event* event,
                                                     bool show) {
  if (!(event->flags() & ui::EF_IS_SYNTHESIZED)) {
    aura::client::CursorClient* client =
        aura::client::GetCursorClient(target->GetRootWindow());
    if (client) {
      if (show && cursor_hidden_by_filter_) {
        cursor_hidden_by_filter_ = false;
        client->ShowCursor(true);
      } else if (client->IsCursorVisible() && !show &&
                 !cursor_hidden_by_filter_) {
        cursor_hidden_by_filter_ = true;
        client->ShowCursor(false);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// CompoundEventFilter, ui::EventHandler implementation:

ui::EventResult CompoundEventFilter::OnKeyEvent(ui::KeyEvent* event) {
  if (ShouldHideCursorOnKeyEvent(*event)) {
    SetCursorVisibilityOnEvent(
        static_cast<aura::Window*>(event->target()), event, false);
  }

  return FilterKeyEvent(event);
}

ui::EventResult CompoundEventFilter::OnMouseEvent(ui::MouseEvent* event) {
  aura::Window* window = static_cast<aura::Window*>(event->target());
  aura::WindowTracker window_tracker;
  window_tracker.Add(window);

  // We must always update the cursor, otherwise the cursor can get stuck if an
  // event filter registered with us consumes the event.
  // It should also update the cursor for clicking and wheels for ChromeOS boot.
  // When ChromeOS is booted, it hides the mouse cursor but immediate mouse
  // operation will show the cursor.
  // We also update the cursor for mouse enter in case a mouse cursor is sent to
  // outside of the root window and moved back for some reasons (e.g. running on
  // on Desktop for testing, or a bug in pointer barrier).
  if (event->type() == ui::ET_MOUSE_ENTERED ||
      event->type() == ui::ET_MOUSE_MOVED ||
      event->type() == ui::ET_MOUSE_PRESSED ||
      event->type() == ui::ET_MOUSEWHEEL) {
    SetCursorVisibilityOnEvent(window, event, true);
    UpdateCursor(window, event);
  }

  ui::EventResult result = FilterMouseEvent(event);
  if ((result & ui::ER_CONSUMED) ||
      !window_tracker.Contains(window) ||
      !window->GetRootWindow()) {
    return result;
  }

  if (event->type() == ui::ET_MOUSE_PRESSED &&
      GetActiveWindow(window) != window) {
    window->GetFocusManager()->SetFocusedWindow(
        FindFocusableWindowFor(window), event);
  }

  return result;
}

ui::EventResult CompoundEventFilter::OnScrollEvent(ui::ScrollEvent* event) {
  return ui::ER_UNHANDLED;
}

ui::EventResult CompoundEventFilter::OnTouchEvent(ui::TouchEvent* event) {
  ui::EventResult result = FilterTouchEvent(event);
  if (result == ui::ER_UNHANDLED &&
      event->type() == ui::ET_TOUCH_PRESSED) {
    SetCursorVisibilityOnEvent(
        static_cast<aura::Window*>(event->target()), event, false);
  }
  return result;
}

ui::EventResult CompoundEventFilter::OnGestureEvent(ui::GestureEvent* event) {
  int result = ui::ER_UNHANDLED;
  if (filters_.might_have_observers()) {
    ObserverListBase<aura::EventFilter>::Iterator it(filters_);
    EventFilter* filter;
    while (!(result & ui::ER_CONSUMED) && (filter = it.GetNext()) != NULL)
      result |= filter->OnGestureEvent(event);
  }

  aura::Window* window = static_cast<aura::Window*>(event->target());
  if (event->type() == ui::ET_GESTURE_BEGIN &&
      event->details().touch_points() == 1 &&
      !(result & ui::ER_CONSUMED) &&
      window->GetRootWindow() &&
      GetActiveWindow(window) != window) {
    window->GetFocusManager()->SetFocusedWindow(
        FindFocusableWindowFor(window), event);
  }

  return static_cast<ui::EventResult>(result);
}

}  // namespace corewm
}  // namespace views
