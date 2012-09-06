// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_ROOT_WINDOW_HOST_LINUX_H_
#define UI_VIEWS_WIDGET_DESKTOP_ROOT_WINDOW_HOST_LINUX_H_

#include "base/basictypes.h"
#include "ui/views/widget/desktop_root_window_host.h"

namespace views {

class DesktopRootWindowHostLinux : public DesktopRootWindowHost {
 public:
  DesktopRootWindowHostLinux();
  virtual ~DesktopRootWindowHostLinux();

 private:
  // Overridden from DesktopRootWindowHost:

  DISALLOW_COPY_AND_ASSIGN(DesktopRootWindowHostLinux);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_ROOT_WINDOW_HOST_LINUX_H_
