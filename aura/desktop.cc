// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/desktop.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "ui/aura/desktop_host.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/root_window.h"
#include "ui/aura/toplevel_window_container.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/gfx/compositor/compositor.h"
#include "ui/gfx/compositor/layer.h"

namespace aura {

// static
Desktop* Desktop::instance_ = NULL;

// static
ui::Compositor*(*Desktop::compositor_factory_)() = NULL;

Desktop::Desktop()
    : default_parent_(NULL),
      host_(aura::DesktopHost::Create(gfx::Rect(200, 200, 1280, 1024))),
      ALLOW_THIS_IN_INITIALIZER_LIST(schedule_paint_(this)),
      active_window_(NULL),
      in_destructor_(false) {
  DCHECK(MessageLoopForUI::current())
      << "The UI message loop must be initialized first.";
  if (compositor_factory_) {
    compositor_ = (*Desktop::compositor_factory())();
  } else {
    compositor_ = ui::Compositor::Create(this, host_->GetAcceleratedWidget(),
                                         host_->GetSize());
  }
  host_->SetDesktop(this);
  DCHECK(compositor_.get());
  window_.reset(new internal::RootWindow);
}

Desktop::~Desktop() {
  in_destructor_ = true;
  if (instance_ == this)
    instance_ = NULL;
}

void Desktop::Init() {
  window_->Init();
  window_->Show();
  compositor()->SetRootLayer(window_->layer());
}

void Desktop::CreateDefaultParentForTesting() {
  Window* default_parent = new internal::ToplevelWindowContainer;
  default_parent->Init();
  default_parent->SetBounds(window_->bounds());
  default_parent->Show();
  window_->AddChild(default_parent);
  set_default_parent(default_parent);
}

void Desktop::Show() {
  host_->Show();
}

void Desktop::SetSize(const gfx::Size& size) {
  host_->SetSize(size);
}

gfx::Size Desktop::GetSize() const {
  return host_->GetSize();
}

void Desktop::SetCursor(gfx::NativeCursor cursor) {
  host_->SetCursor(cursor);
}

void Desktop::Run() {
  Show();
  MessageLoopForUI::current()->Run(host_.get());
}

void Desktop::Draw() {
  compositor_->Draw(false);
}

bool Desktop::OnMouseEvent(const MouseEvent& event) {
  return window_->HandleMouseEvent(event);
}

bool Desktop::OnKeyEvent(const KeyEvent& event) {
  return window_->HandleKeyEvent(event);
}

void Desktop::OnHostResized(const gfx::Size& size) {
  gfx::Rect bounds(0, 0, size.width(), size.height());
  compositor_->WidgetSizeChanged(size);
  window_->SetBounds(bounds);
}

void Desktop::ScheduleDraw() {
  if (schedule_paint_.empty()) {
    MessageLoop::current()->PostTask(FROM_HERE,
        schedule_paint_.NewRunnableMethod(&Desktop::Draw));
  }
}

void Desktop::SetActiveWindow(Window* window, Window* to_focus) {
  if (active_window_ == window)
    return;
  if (active_window_)
    active_window_->delegate()->OnLostActive();
  active_window_ = window;
  if (active_window_) {
    active_window_->parent()->MoveChildToFront(active_window_);
    active_window_->delegate()->OnActivated();
    active_window_->GetFocusManager()->SetFocusedWindow(
        to_focus ? to_focus : active_window_);
  }
}

void Desktop::ActivateTopmostWindow() {
  SetActiveWindow(GetTopmostWindowToActivate(NULL), NULL);
}

void Desktop::WindowDestroying(Window* window) {
  if (in_destructor_ || window != active_window_)
    return;

  // Reset active_window_ before invoking SetActiveWindow so that we don't
  // attempt to notify it while running its destructor.
  active_window_ = NULL;
  SetActiveWindow(GetTopmostWindowToActivate(window), NULL);
}

bool Desktop::DispatchNativeEvent(const base::NativeEvent& event) {
  // TODO(oshima): consolidate windows and linux.
#if defined(OS_WIN)
  return host_->Dispatch(event);
#else
  return host_->Dispatch(event) != base::MessagePumpDispatcher::EVENT_IGNORED;
#endif
}

Window* Desktop::GetTopmostWindowToActivate(Window* ignore) {
  Window::Windows windows(default_parent_->children());
  for (Window::Windows::const_reverse_iterator i = windows.rbegin();
       i != windows.rend(); ++i) {
    if (*i != ignore && (*i)->IsVisible() &&
        (*i)->delegate()->ShouldActivate(NULL))
      return *i;
  }
  return NULL;
}

// static
Desktop* Desktop::GetInstance() {
  if (!instance_) {
    instance_ = new Desktop;
    instance_->Init();
  }
  return instance_;
}

}  // namespace aura
