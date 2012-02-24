// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_ROOT_WINDOW_HOST_LINUX_H_
#define UI_AURA_ROOT_WINDOW_HOST_LINUX_H_
#pragma once

#include <X11/Xlib.h>

// Get rid of a macro from Xlib.h that conflicts with Aura's RootWindow class.
#undef RootWindow

#include "base/message_loop.h"
#include "ui/aura/root_window_host.h"
#include "ui/gfx/rect.h"

namespace aura {

class RootWindowHostLinux : public RootWindowHost,
                            public MessageLoop::DestructionObserver {
 public:
  explicit RootWindowHostLinux(const gfx::Rect& bounds);
  virtual ~RootWindowHostLinux();

 private:
  // MessageLoop::Dispatcher Override.
  virtual DispatchStatus Dispatch(XEvent* xev) OVERRIDE;

  // RootWindowHost Overrides.
  virtual void SetRootWindow(RootWindow* root_window) OVERRIDE;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void ToggleFullScreen() OVERRIDE;
  virtual gfx::Size GetSize() const OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Point GetLocationOnNativeScreen() const OVERRIDE;
  virtual void SetCapture() OVERRIDE;
  virtual void ReleaseCapture() OVERRIDE;
  virtual void SetCursor(gfx::NativeCursor cursor_type) OVERRIDE;
  virtual void ShowCursor(bool show) OVERRIDE;
  virtual gfx::Point QueryMouseLocation() OVERRIDE;
  virtual bool ConfineCursorToRootWindow() OVERRIDE;
  virtual void UnConfineCursor() OVERRIDE;
  virtual void MoveCursorTo(const gfx::Point& location) OVERRIDE;
  virtual void PostNativeEvent(const base::NativeEvent& event) OVERRIDE;
  virtual MessageLoop::Dispatcher* GetDispatcher() OVERRIDE;

  // MessageLoop::DestructionObserver Overrides.
  virtual void WillDestroyCurrentMessageLoop() OVERRIDE;

  // Returns true if there's an X window manager present... in most cases.  Some
  // window managers (notably, ion3) don't implement enough of ICCCM for us to
  // detect that they're there.
  bool IsWindowManagerPresent();

  // Sets the cursor on |xwindow_| to |cursor|.  Does not check or update
  // |current_cursor_|.
  void SetCursorInternal(gfx::NativeCursor cursor);

  RootWindow* root_window_;

  // The display and the native X window hosting the root window.
  Display* xdisplay_;
  ::Window xwindow_;

  // The native root window.
  ::Window x_root_window_;

  // Current Aura cursor.
  gfx::NativeCursor current_cursor_;

  // Is the cursor currently shown?
  bool cursor_shown_;

  // The invisible cursor.
  ::Cursor invisible_cursor_;

  // The bounds of |xwindow_|.
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowHostLinux);
};

}  // namespace aura

#endif  // UI_AURA_ROOT_WINDOW_HOST_LINUX_H_
