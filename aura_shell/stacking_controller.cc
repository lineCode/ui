// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura_shell/stacking_controller.h"

#include "ui/aura/desktop.h"
#include "ui/aura/window.h"
#include "ui/aura_shell/shell.h"
#include "ui/aura_shell/shell_window_ids.h"

namespace aura_shell {
namespace internal {
namespace {

aura::Window* GetContainer(int id) {
  return Shell::GetInstance()->GetContainer(id);
}

// Returns true if children of |window| can be activated.
bool SupportsChildActivation(aura::Window* window) {
  return window->id() == kShellWindowId_DefaultContainer ||
         window->id() == kShellWindowId_AlwaysOnTopContainer;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// StackingController, public:

StackingController::StackingController() {
  aura::Desktop::GetInstance()->SetStackingClient(this);
}

StackingController::~StackingController() {
}

// static
aura::Window* StackingController::GetActivatableWindow(aura::Window* window) {
  aura::Window* parent = window->parent();
  aura::Window* child = window;
  while (parent) {
    if (SupportsChildActivation(parent))
      return child;
    parent = parent->parent();
    child = child->parent();
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// StackingController, aura::StackingClient implementation:

void StackingController::AddChildToDefaultParent(aura::Window* window) {
  aura::Window* parent = NULL;
  switch (window->type()) {
    case aura::WINDOW_TYPE_NORMAL:
    case aura::WINDOW_TYPE_POPUP:
      parent = GetContainer(internal::kShellWindowId_DefaultContainer);
      break;
    case aura::WINDOW_TYPE_MENU:
    case aura::WINDOW_TYPE_TOOLTIP:
      parent = GetContainer(internal::kShellWindowId_MenusAndTooltipsContainer);
      break;
    default:
      NOTREACHED() << "Window " << window->id()
                   << " has unhandled type " << window->type();
      break;
  }
  parent->AddChild(window);
}

bool StackingController::CanActivateWindow(aura::Window* window) const {
  return window && SupportsChildActivation(window->parent());
}

aura::Window* StackingController::GetTopmostWindowToActivate(
    aura::Window* ignore) const {
  const aura::Window* container = GetContainer(kShellWindowId_DefaultContainer);
  for (aura::Window::Windows::const_reverse_iterator i =
           container->children().rbegin();
       i != container->children().rend();
       ++i) {
    if (*i != ignore && (*i)->CanActivate())
      return *i;
  }
  return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// StackingController, private:

}  // namespace internal
}  // namespace aura_shell
