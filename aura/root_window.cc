// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/root_window.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "ui/aura/aura_switches.h"
#include "ui/aura/client/activation_client.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/client/stacking_client.h"
#include "ui/aura/client/tooltip_client.h"
#include "ui/aura/client/window_drag_drop_delegate.h"
#include "ui/aura/root_window_host.h"
#include "ui/aura/root_window_observer.h"
#include "ui/aura/event.h"
#include "ui/aura/event_filter.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/screen_aura.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/compositor/compositor.h"
#include "ui/gfx/compositor/layer.h"
#include "ui/gfx/compositor/layer_animator.h"

#ifdef USE_WEBKIT_COMPOSITOR
#include "ui/gfx/compositor/compositor_cc.h"
#endif

using std::string;
using std::vector;

namespace aura {

namespace {

// Default bounds for the host window.
static const int kDefaultHostWindowX = 200;
static const int kDefaultHostWindowY = 200;
static const int kDefaultHostWindowWidth = 1280;
static const int kDefaultHostWindowHeight = 1024;

// Returns true if |target| has a non-client (frame) component at |location|,
// in window coordinates.
bool IsNonClientLocation(Window* target, const gfx::Point& location) {
  if (!target->delegate())
    return false;
  int hit_test_code = target->delegate()->GetNonClientComponent(location);
  return hit_test_code != HTCLIENT && hit_test_code != HTNOWHERE;
}

class DefaultStackingClient : public StackingClient {
 public:
  explicit DefaultStackingClient(RootWindow* root_window)
      : root_window_(root_window) {}
  virtual ~DefaultStackingClient() {}

 private:

  // Overridden from StackingClient:
  virtual void AddChildToDefaultParent(Window* window) OVERRIDE {
    root_window_->AddChild(window);
  }

  RootWindow* root_window_;

  DISALLOW_COPY_AND_ASSIGN(DefaultStackingClient);
};

typedef std::vector<EventFilter*> EventFilters;

void GetEventFiltersToNotify(Window* target, EventFilters* filters) {
  Window* window = target->parent();
  while (window) {
    if (window->event_filter())
      filters->push_back(window->event_filter());
    window = window->parent();
  }
}

}  // namespace

RootWindow* RootWindow::instance_ = NULL;
bool RootWindow::use_fullscreen_host_window_ = false;

////////////////////////////////////////////////////////////////////////////////
// RootWindow, public:

// static
RootWindow* RootWindow::GetInstance() {
  if (!instance_) {
    instance_ = new RootWindow;
    instance_->Init();
  }
  return instance_;
}

// static
void RootWindow::DeleteInstance() {
  delete instance_;
  instance_ = NULL;
}

void RootWindow::SetStackingClient(StackingClient* stacking_client) {
  stacking_client_.reset(stacking_client);
}

void RootWindow::ShowRootWindow() {
  host_->Show();
}

void RootWindow::SetHostSize(const gfx::Size& size) {
  host_->SetSize(size);
  // Requery the location to constrain it within the new root window size.
  last_mouse_location_ = host_->QueryMouseLocation();
}

gfx::Size RootWindow::GetHostSize() const {
  gfx::Rect rect(host_->GetSize());
  layer()->transform().TransformRect(&rect);
  return rect.size();
}

void RootWindow::SetCursor(gfx::NativeCursor cursor) {
  last_cursor_ = cursor;
  // A lot of code seems to depend on NULL cursors actually showing an arrow,
  // so just pass everything along to the host.
  host_->SetCursor(cursor);
}

void RootWindow::Run() {
  ShowRootWindow();
  MessageLoopForUI::current()->Run();
}

void RootWindow::Draw() {
  compositor_->Draw(false);
}

bool RootWindow::DispatchMouseEvent(MouseEvent* event) {
  static const int kMouseButtonFlagMask =
      ui::EF_LEFT_BUTTON_DOWN |
      ui::EF_MIDDLE_BUTTON_DOWN |
      ui::EF_RIGHT_BUTTON_DOWN;

  event->UpdateForTransform(layer()->transform());

  last_mouse_location_ = event->location();

  Window* target =
      mouse_pressed_handler_ ? mouse_pressed_handler_ : capture_window_;
  if (!target)
    target = GetEventHandlerForPoint(event->location());
  switch (event->type()) {
    case ui::ET_MOUSE_MOVED:
      HandleMouseMoved(*event, target);
      break;
    case ui::ET_MOUSE_PRESSED:
      if (!mouse_pressed_handler_)
        mouse_pressed_handler_ = target;
      mouse_button_flags_ = event->flags() & kMouseButtonFlagMask;
      break;
    case ui::ET_MOUSE_RELEASED:
      mouse_pressed_handler_ = NULL;
      mouse_button_flags_ = event->flags() & kMouseButtonFlagMask;
      break;
    default:
      break;
  }
  if (target && target->delegate()) {
    int flags = event->flags();
    gfx::Point location_in_window = event->location();
    Window::ConvertPointToWindow(this, target, &location_in_window);
    if (IsNonClientLocation(target, location_in_window))
      flags |= ui::EF_IS_NON_CLIENT;
    MouseEvent translated_event(*event, this, target, event->type(), flags);
    return ProcessMouseEvent(target, &translated_event);
  }
  return false;
}

bool RootWindow::DispatchKeyEvent(KeyEvent* event) {
  if (focused_window_) {
    KeyEvent translated_event(*event);
    return ProcessKeyEvent(focused_window_, &translated_event);
  }
  return false;
}

bool RootWindow::DispatchTouchEvent(TouchEvent* event) {
  event->UpdateForTransform(layer()->transform());
  bool handled = false;
  Window* target =
      touch_event_handler_ ? touch_event_handler_ : capture_window_;
  if (!target)
    target = GetEventHandlerForPoint(event->location());
  if (target) {
    TouchEvent translated_event(*event, this, target);
    ui::TouchStatus status = ProcessTouchEvent(target, &translated_event);
    if (status == ui::TOUCH_STATUS_START)
      touch_event_handler_ = target;
    else if (status == ui::TOUCH_STATUS_END ||
             status == ui::TOUCH_STATUS_CANCEL)
      touch_event_handler_ = NULL;
    handled = status != ui::TOUCH_STATUS_UNKNOWN;
  }
  return handled;
}

void RootWindow::OnHostResized(const gfx::Size& size) {
  // The compositor should have the same size as the native root window host.
  compositor_->WidgetSizeChanged(size);

  // The layer, and all the observers should be notified of the
  // transformed size of the root window.
  gfx::Rect bounds(size);
  layer()->transform().TransformRect(&bounds);
  SetBounds(gfx::Rect(bounds.size()));
  FOR_EACH_OBSERVER(RootWindowObserver, observers_,
                    OnRootWindowResized(bounds.size()));
}

void RootWindow::OnNativeScreenResized(const gfx::Size& size) {
  if (use_fullscreen_host_window_)
    SetHostSize(size);
}

void RootWindow::WindowInitialized(Window* window) {
  FOR_EACH_OBSERVER(RootWindowObserver, observers_,
                    OnWindowInitialized(window));
}

void RootWindow::WindowDestroying(Window* window) {
  // Update the focused window state if the window was focused.
  if (focused_window_ == window)
    SetFocusedWindow(NULL);

  // When a window is being destroyed it's likely that the WindowDelegate won't
  // want events, so we reset the mouse_pressed_handler_ and capture_window_ and
  // don't sent it release/capture lost events.
  if (mouse_pressed_handler_ == window)
    mouse_pressed_handler_ = NULL;
  if (mouse_moved_handler_ == window)
    mouse_moved_handler_ = NULL;
  if (capture_window_ == window)
    capture_window_ = NULL;
  if (touch_event_handler_ == window)
    touch_event_handler_ = NULL;
}

MessageLoop::Dispatcher* RootWindow::GetDispatcher() {
  return host_.get();
}

void RootWindow::AddRootWindowObserver(RootWindowObserver* observer) {
  observers_.AddObserver(observer);
}

void RootWindow::RemoveRootWindowObserver(RootWindowObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool RootWindow::IsMouseButtonDown() const {
  return mouse_button_flags_ != 0;
}

void RootWindow::PostNativeEvent(const base::NativeEvent& native_event) {
  host_->PostNativeEvent(native_event);
}

void RootWindow::ConvertPointToNativeScreen(gfx::Point* point) const {
  gfx::Point location = host_->GetLocationOnNativeScreen();
  point->Offset(location.x(), location.y());
}

void RootWindow::SetCapture(Window* window) {
  if (capture_window_ == window)
    return;

  if (capture_window_ && capture_window_->delegate())
    capture_window_->delegate()->OnCaptureLost();
  capture_window_ = window;

  if (capture_window_) {
    // Make all subsequent mouse events and touch go to the capture window. We
    // shouldn't need to send an event here as OnCaptureLost should take care of
    // that.
    if (touch_event_handler_)
      touch_event_handler_ = capture_window_;
    if (mouse_moved_handler_ || mouse_button_flags_ != 0)
      mouse_moved_handler_ = capture_window_;
  } else {
    // When capture is lost, we must reset the event handlers.
    touch_event_handler_ = NULL;
    mouse_moved_handler_ = NULL;
  }
  mouse_pressed_handler_ = NULL;
}

void RootWindow::ReleaseCapture(Window* window) {
  if (capture_window_ != window)
    return;
  SetCapture(NULL);
}

void RootWindow::SetTransform(const ui::Transform& transform) {
  Window::SetTransform(transform);

  // If the layer is not animating, then we need to update the host size
  // immediately.
  if (!layer()->GetAnimator()->is_animating())
    OnHostResized(host_->GetSize());
}

#if !defined(NDEBUG)
void RootWindow::ToggleFullScreen() {
  host_->ToggleFullScreen();
}
#endif

////////////////////////////////////////////////////////////////////////////////
// RootWindow, private:

RootWindow::RootWindow()
    : Window(NULL),
      host_(aura::RootWindowHost::Create(GetInitialHostWindowBounds())),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          stacking_client_(new DefaultStackingClient(this))),
      ALLOW_THIS_IN_INITIALIZER_LIST(schedule_paint_factory_(this)),
      mouse_button_flags_(0),
      last_cursor_(kCursorNull),
      screen_(new ScreenAura),
      capture_window_(NULL),
      mouse_pressed_handler_(NULL),
      mouse_moved_handler_(NULL),
      focused_window_(NULL),
      touch_event_handler_(NULL) {
  SetName("RootWindow");
  gfx::Screen::SetInstance(screen_);
  host_->SetRootWindow(this);
  last_mouse_location_ = host_->QueryMouseLocation();

  if (ui::Compositor::compositor_factory()) {
    compositor_ = (*ui::Compositor::compositor_factory())(this);
  } else {
#ifdef USE_WEBKIT_COMPOSITOR
    ui::CompositorCC::Initialize(false);
#endif
    compositor_ = ui::Compositor::Create(this, host_->GetAcceleratedWidget(),
                                         host_->GetSize());
  }
  DCHECK(compositor_.get());
}

RootWindow::~RootWindow() {
  // Make sure to destroy the compositor before terminating so that state is
  // cleared and we don't hit asserts.
  compositor_ = NULL;
  // An observer may have been added by an animation on the RootWindow.
  layer()->GetAnimator()->RemoveObserver(this);
#ifdef USE_WEBKIT_COMPOSITOR
  if (!ui::Compositor::compositor_factory())
    ui::CompositorCC::Terminate();
#endif
  if (instance_ == this)
    instance_ = NULL;
}

void RootWindow::HandleMouseMoved(const MouseEvent& event, Window* target) {
  if (target == mouse_moved_handler_)
    return;

  // Send an exited event.
  if (mouse_moved_handler_ && mouse_moved_handler_->delegate()) {
    MouseEvent translated_event(event, this, mouse_moved_handler_,
                                ui::ET_MOUSE_EXITED, event.flags());
    ProcessMouseEvent(mouse_moved_handler_, &translated_event);
  }
  mouse_moved_handler_ = target;
  // Send an entered event.
  if (mouse_moved_handler_ && mouse_moved_handler_->delegate()) {
    MouseEvent translated_event(event, this, mouse_moved_handler_,
                                ui::ET_MOUSE_ENTERED, event.flags());
    ProcessMouseEvent(mouse_moved_handler_, &translated_event);
  }
}

bool RootWindow::ProcessMouseEvent(Window* target, MouseEvent* event) {
  if (!target->IsVisible())
    return false;

  EventFilters filters;
  GetEventFiltersToNotify(target, &filters);
  for (EventFilters::const_reverse_iterator it = filters.rbegin();
       it != filters.rend(); ++it) {
    if ((*it)->PreHandleMouseEvent(target, event))
      return true;
  }

  return target->delegate()->OnMouseEvent(event);
}

bool RootWindow::ProcessKeyEvent(Window* target, KeyEvent* event) {
  if (!target->IsVisible())
    return false;

  EventFilters filters;
  GetEventFiltersToNotify(target, &filters);
  for (EventFilters::const_reverse_iterator it = filters.rbegin();
       it != filters.rend(); ++it) {
    if ((*it)->PreHandleKeyEvent(target, event))
      return true;
  }

  return target->delegate()->OnKeyEvent(event);
}

ui::TouchStatus RootWindow::ProcessTouchEvent(Window* target,
                                              TouchEvent* event) {
  if (!target->IsVisible())
    return ui::TOUCH_STATUS_UNKNOWN;

  EventFilters filters;
  GetEventFiltersToNotify(target, &filters);
  for (EventFilters::const_reverse_iterator it = filters.rbegin();
       it != filters.rend(); ++it) {
    ui::TouchStatus status = (*it)->PreHandleTouchEvent(target, event);
    if (status != ui::TOUCH_STATUS_UNKNOWN)
      return status;
  }

  return target->delegate()->OnTouchEvent(event);
}

void RootWindow::ScheduleDraw() {
  if (!schedule_paint_factory_.HasWeakPtrs()) {
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&RootWindow::Draw, schedule_paint_factory_.GetWeakPtr()));
  }
}

bool RootWindow::CanFocus() const {
  return IsVisible();
}

internal::FocusManager* RootWindow::GetFocusManager() {
  return this;
}

RootWindow* RootWindow::GetRootWindow() {
  return this;
}

void RootWindow::WindowDetachedFromRootWindow(Window* detached) {
  DCHECK(capture_window_ != this);

  // If the ancestor of the capture window is detached,
  // release the capture.
  if (detached->Contains(capture_window_) && detached != this)
    ReleaseCapture(capture_window_);

  // If the ancestor of the focused window is detached,
  // release the focus.
  if (detached->Contains(focused_window_))
    SetFocusedWindow(NULL);

  // If the ancestor of any event handler windows are detached, release the
  // pointer to those windows.
  if (detached->Contains(mouse_pressed_handler_))
    mouse_pressed_handler_ = NULL;
  if (detached->Contains(mouse_moved_handler_))
    mouse_moved_handler_ = NULL;
  if (detached->Contains(touch_event_handler_))
    touch_event_handler_ = NULL;
}

void RootWindow::OnLayerAnimationEnded(
    const ui::LayerAnimationSequence* animation) {
  OnHostResized(host_->GetSize());
}

void RootWindow::OnLayerAnimationScheduled(
    const ui::LayerAnimationSequence* animation) {
}

void RootWindow::OnLayerAnimationAborted(
    const ui::LayerAnimationSequence* animation) {
}

void RootWindow::SetFocusedWindow(Window* focused_window) {
  if (focused_window == focused_window_)
    return;
  if (focused_window && !focused_window->CanFocus())
    return;
  // The NULL-check of |focused)window| is essential here before asking the
  // activation client, since it is valid to clear the focus by calling
  // SetFocusedWindow() to NULL.
  if (focused_window && ActivationClient::GetActivationClient() &&
      !ActivationClient::GetActivationClient()->CanFocusWindow(
          focused_window)) {
    return;
  }

  if (focused_window_ && focused_window_->delegate())
    focused_window_->delegate()->OnBlur();
  focused_window_ = focused_window;
  if (focused_window_ && focused_window_->delegate())
    focused_window_->delegate()->OnFocus();
  if (focused_window_) {
    FOR_EACH_OBSERVER(RootWindowObserver, observers_,
                      OnWindowFocused(focused_window_));
  }
}

Window* RootWindow::GetFocusedWindow() {
  return focused_window_;
}

bool RootWindow::IsFocusedWindow(const Window* window) const {
  return focused_window_ == window;
}

void RootWindow::Init() {
  Window::Init(ui::Layer::LAYER_HAS_NO_TEXTURE);
  SetBounds(gfx::Rect(host_->GetSize()));
  Show();
  compositor()->SetRootLayer(layer());
}

gfx::Rect RootWindow::GetInitialHostWindowBounds() const {
  gfx::Rect bounds(kDefaultHostWindowX, kDefaultHostWindowY,
                   kDefaultHostWindowWidth, kDefaultHostWindowHeight);

  const string size_str = CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      switches::kAuraHostWindowSize);
  vector<string> parts;
  base::SplitString(size_str, 'x', &parts);
  int parsed_width = 0, parsed_height = 0;
  if (parts.size() == 2 &&
      base::StringToInt(parts[0], &parsed_width) && parsed_width > 0 &&
      base::StringToInt(parts[1], &parsed_height) && parsed_height > 0) {
    bounds.set_size(gfx::Size(parsed_width, parsed_height));
  } else if (use_fullscreen_host_window_) {
    bounds = gfx::Rect(RootWindowHost::GetNativeScreenSize());
  }

  return bounds;
}

}  // namespace aura
