// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_ENV_H_
#define UI_AURA_ENV_H_
#pragma once

#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/observer_list.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/client/stacking_client.h"

namespace aura {

class EnvObserver;
class Window;

#if !defined(OS_MACOSX)
class Dispatcher : public MessageLoop::Dispatcher {
 public:
  virtual ~Dispatcher() {}
};

// Creates a platform-specific native event dispatcher.
Dispatcher* CreateDispatcher();
#endif

// A singleton object that tracks general state within Aura.
// TODO(beng): manage RootWindows.
class AURA_EXPORT Env {
 public:
  Env();
  ~Env();

  static Env* GetInstance();
  static void DeleteInstance();

  void AddObserver(EnvObserver* observer);
  void RemoveObserver(EnvObserver* observer);

  bool is_mouse_button_down() const { return mouse_button_flags_ != 0; }
  void set_mouse_button_flags(int mouse_button_flags) {
    mouse_button_flags_ = mouse_button_flags;
  }

  client::StackingClient* stacking_client() { return stacking_client_; }
  void set_stacking_client(client::StackingClient* stacking_client) {
    stacking_client_ = stacking_client;
  }

  // Returns the native event dispatcher. The result should only be passed to
  // MessageLoopForUI::RunWithDispatcher() or
  // MessageLoopForUI::RunAllPendingWithDispatcher(), or used to dispatch
  // an event by |Dispatch(const NativeEvent&)| on it. It must never be stored.
#if !defined(OS_MACOSX)
  MessageLoop::Dispatcher* GetDispatcher();
#endif

 private:
  friend class Window;

  // Called by the Window when it is initialized. Notifies observers.
  void NotifyWindowInitialized(Window* window);

  ObserverList<EnvObserver> observers_;
#if !defined(OS_MACOSX)
  scoped_ptr<Dispatcher> dispatcher_;
#endif

  static Env* instance_;
  int mouse_button_flags_;
  client::StackingClient* stacking_client_;

  DISALLOW_COPY_AND_ASSIGN(Env);
};

}  // namespace aura

#endif  // UI_AURA_ENV_H_
