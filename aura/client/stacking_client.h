// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_CLIENT_STACKING_CLIENT_H_
#define UI_AURA_CLIENT_STACKING_CLIENT_H_
#pragma once

#include "ui/aura/aura_export.h"

namespace aura {

class Window;

// An interface implemented by an object that stacks windows.
class AURA_EXPORT StackingClient {
 public:
  virtual ~StackingClient() {}

  // Called by the Window when its parent is set to NULL. The delegate is given
  // an opportunity to inspect the window and add it to a default parent window
  // of its choosing.
  virtual void AddChildToDefaultParent(Window* window) = 0;
};

}  // namespace aura

#endif  // UI_AURA_CLIENT_STACKING_CLIENT_H_
