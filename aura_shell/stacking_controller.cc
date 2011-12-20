// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura_shell/stacking_controller.h"

#include "ui/aura/client/aura_constants.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura_shell/always_on_top_controller.h"
#include "ui/aura_shell/shell.h"
#include "ui/aura_shell/shell_window_ids.h"

namespace aura_shell {
namespace internal {
namespace {

aura::Window* GetContainer(int id) {
  return Shell::GetInstance()->GetContainer(id);
}

bool IsWindowModal(aura::Window* window) {
  return window->transient_parent() &&
      window->GetIntProperty(aura::client::kModalKey);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// StackingController, public:

StackingController::StackingController() {
  aura::client::SetStackingClient(this);
  always_on_top_controller_.reset(new internal::AlwaysOnTopController);
  always_on_top_controller_->SetContainers(
      GetContainer(internal::kShellWindowId_DefaultContainer),
      GetContainer(internal::kShellWindowId_AlwaysOnTopContainer));
}

StackingController::~StackingController() {
}

////////////////////////////////////////////////////////////////////////////////
// StackingController, aura::StackingClient implementation:

aura::Window* StackingController::GetDefaultParent(aura::Window* window) {
  switch (window->type()) {
    case aura::client::WINDOW_TYPE_NORMAL:
    case aura::client::WINDOW_TYPE_POPUP:
      if (IsWindowModal(window))
        return GetModalContainer(window);
      return always_on_top_controller_->GetContainer(window);
    case aura::client::WINDOW_TYPE_MENU:
    case aura::client::WINDOW_TYPE_TOOLTIP:
      return GetContainer(internal::kShellWindowId_MenusAndTooltipsContainer);
    default:
      NOTREACHED() << "Window " << window->id()
                   << " has unhandled type " << window->type();
      break;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// StackingController, private:

aura::Window* StackingController::GetModalContainer(
    aura::Window* window) const {
  if (!IsWindowModal(window))
    return NULL;

  // If screen lock is not active, all modal windows are placed into the
  // normal modal container.
  aura::Window* lock_container =
      GetContainer(internal::kShellWindowId_LockScreenContainer);
  if (!lock_container->children().size())
    return GetContainer(internal::kShellWindowId_ModalContainer);

  // Otherwise those that originate from LockScreen container and above are
  // placed in the screen lock modal container.
  int lock_container_id = lock_container->id();
  int window_container_id = window->transient_parent()->parent()->id();

  aura::Window* container = NULL;
  if (window_container_id < lock_container_id)
    container = GetContainer(internal::kShellWindowId_ModalContainer);
  else
    container = GetContainer(internal::kShellWindowId_LockModalContainer);

  return container;
}

}  // namespace internal
}  // namespace aura_shell
