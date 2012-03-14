// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SURFACE_ACCELERATED_SURFACE_WIN_H_
#define UI_GFX_SURFACE_ACCELERATED_SURFACE_WIN_H_
#pragma once

#include <d3d9.h>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/win/scoped_comptr.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
#include "ui/gfx/surface/surface_export.h"

class PresentThread;

class AcceleratedPresenter
    : public base::RefCountedThreadSafe<AcceleratedPresenter> {
 public:
  typedef base::Callback<void(bool)> CompletionTaskl;

  AcceleratedPresenter();

  // The public member functions are called on the main thread.
  void AsyncPresentAndAcknowledge(
      gfx::NativeWindow window,
      const gfx::Size& size,
      int64 surface_id,
      const base::Callback<void(bool)>& completion_task);
  bool Present(gfx::NativeWindow window);
  void Suspend();
  void WaitForPendingTasks();

 private:
  friend class base::RefCountedThreadSafe<AcceleratedPresenter>;

  ~AcceleratedPresenter();

  // These member functions are called on the PresentThread with which the
  // presenter has affinity.
  void DoPresentAndAcknowledge(
      gfx::NativeWindow window,
      const gfx::Size& size,
      int64 surface_id,
      const base::Callback<void(bool)>& completion_task);
  void DoSuspend();

  // The thread with which this presenter has affinity.
  PresentThread* const present_thread_;

  // The lock is taken while any thread is calling the object, except those that
  // simply post from the main thread to the present thread via the immutable
  // present_thread_ member.
  base::Lock lock_;

  // The current size of the swap chain. This is only accessed on the thread
  // with which the surface has affinity.
  gfx::Size size_;

  // The swap chain is presented to the child window. Copy semantics
  // are used so it is possible to represent it to quickly validate the window.
  base::win::ScopedComPtr<IDirect3DSwapChain9> swap_chain_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratedPresenter);
};

class SURFACE_EXPORT AcceleratedSurface {
 public:
  AcceleratedSurface();
  ~AcceleratedSurface();

  // Schedule a frame to be presented. The completion callback will be invoked
  // when it is safe to write to the surface on another thread. The lock for
  // this surface will be held while the completion callback runs.
  void AsyncPresentAndAcknowledge(
      gfx::NativeWindow window,
      const gfx::Size& size,
      int64 surface_id,
      const base::Callback<void(bool)>& completion_task);

  // Synchronously present a frame with no acknowledgement.
  bool Present(gfx::NativeWindow window);

  // Temporarily release resources until a new surface is asynchronously
  // presented. Present will not be able to represent the last surface after
  // calling this and will return false.
  void Suspend();

 private:
  // Immutable and accessible on any thread.
  const scoped_refptr<AcceleratedPresenter> presenter_;
  DISALLOW_COPY_AND_ASSIGN(AcceleratedSurface);
};

#endif  // UI_GFX_SURFACE_ACCELERATED_SURFACE_WIN_H_
