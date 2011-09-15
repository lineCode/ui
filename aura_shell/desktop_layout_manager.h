// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SHELL_DESKTOP_LAYOUT_MANAGER_H_
#define UI_AURA_SHELL_DESKTOP_LAYOUT_MANAGER_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/aura/layout_manager.h"

namespace aura {
class Window;
}
namespace views {
class Widget;
}

class DesktopLayoutManager : public aura::LayoutManager {
 public:
  explicit DesktopLayoutManager(aura::Window* owner);
  virtual ~DesktopLayoutManager();

  void set_background_widget(views::Widget* background_widget) {
    background_widget_ = background_widget;
  }

 private:
  // Overridden from aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE;

  aura::Window* owner_;
  views::Widget* background_widget_;

  DISALLOW_COPY_AND_ASSIGN(DesktopLayoutManager);
};

#endif  // UI_AURA_SHELL_DESKTOP_LAYOUT_MANAGER_H_
