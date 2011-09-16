// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura_shell/desktop_layout_manager.h"

#include "ui/aura/window.h"
#include "views/widget/widget.h"

namespace aura_shell {
namespace internal {

////////////////////////////////////////////////////////////////////////////////
// DesktopLayoutManager, public:

DesktopLayoutManager::DesktopLayoutManager(aura::Window* owner)
    : owner_(owner),
      background_widget_(NULL),
      launcher_widget_(NULL) {
}

DesktopLayoutManager::~DesktopLayoutManager() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopLayoutManager, aura::LayoutManager implementation:

void DesktopLayoutManager::OnWindowResized() {
  background_widget_->SetBounds(
      gfx::Rect(owner_->bounds().width(), owner_->bounds().height()));

  gfx::Rect launcher_bounds = launcher_widget_->GetWindowScreenBounds();
  launcher_widget_->SetBounds(
      gfx::Rect(owner_->bounds().width() / 2 - launcher_bounds.width() / 2,
                owner_->bounds().bottom() - launcher_bounds.height(),
                launcher_bounds.width(),
                launcher_bounds.height()));
}

}  // namespace internal
}  // namespace aura_shell
