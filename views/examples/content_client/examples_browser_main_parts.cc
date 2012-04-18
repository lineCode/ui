// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/content_client/examples_browser_main_parts.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/content_switches.h"
#include "content/shell/shell.h"
#include "content/shell/shell_browser_context.h"
#include "content/shell/shell_devtools_delegate.h"
#include "content/shell/shell_switches.h"
#include "googleurl/src/gurl.h"
#include "net/base/net_module.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/views/examples/examples_window.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/focus/accelerator_handler.h"

#if defined(USE_AURA)
#include "ui/aura/desktop/desktop_stacking_client.h"
#include "ui/aura/env.h"
#include "ui/views/widget/native_widget_aura.h"
#endif

namespace views {
namespace examples {

ExamplesBrowserMainParts::ExamplesBrowserMainParts(
    const content::MainFunctionParams& parameters)
    : BrowserMainParts(),
      devtools_delegate_(NULL) {
}

ExamplesBrowserMainParts::~ExamplesBrowserMainParts() {
}

#if !defined(OS_MACOSX)
void ExamplesBrowserMainParts::PreMainMessageLoopStart() {
}
#endif

int ExamplesBrowserMainParts::PreCreateThreads() {
  return 0;
}

void ExamplesBrowserMainParts::PreMainMessageLoopRun() {
  browser_context_.reset(new content::ShellBrowserContext);

#if defined(USE_AURA)
  stacking_client_.reset(new aura::DesktopStackingClient);
#endif
  views_delegate_.reset(new views::TestViewsDelegate);

  views::examples::ShowExamplesWindow(views::examples::QUIT_ON_CLOSE,
                                      browser_context_.get());
}

void ExamplesBrowserMainParts::PostMainMessageLoopRun() {
  if (devtools_delegate_)
    devtools_delegate_->Stop();
  browser_context_.reset();
  views_delegate_.reset();
#if defined(USE_AURA)
  stacking_client_.reset();
  aura::Env::DeleteInstance();
#endif
}

bool ExamplesBrowserMainParts::MainMessageLoopRun(int* result_code) {
  // xxx: Hax here because this kills event handling.
#if !defined(USE_AURA)
  views::AcceleratorHandler accelerator_handler;
  MessageLoopForUI::current()->RunWithDispatcher(&accelerator_handler);
#else
  MessageLoopForUI::current()->Run();
#endif
  return true;
}

ui::Clipboard* ExamplesBrowserMainParts::GetClipboard() {
  if (!clipboard_.get())
    clipboard_.reset(new ui::Clipboard());
  return clipboard_.get();
}

}  // namespace examples
}  // namespace views
