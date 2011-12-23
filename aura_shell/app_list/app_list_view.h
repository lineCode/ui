// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SHELL_APP_LIST_APP_LIST_VIEW_H_
#define UI_AURA_SHELL_APP_LIST_APP_LIST_VIEW_H_
#pragma once

#include "base/memory/scoped_ptr.h"
#include "ui/aura_shell/app_list/app_list_item_view_listener.h"
#include "ui/aura_shell/aura_shell_export.h"
#include "ui/aura_shell/shell_delegate.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class View;
}

namespace aura_shell {

class AppListModel;
class AppListViewDelegate;

// AppListView is the top-level view and controller of app list UI. It creates
// and hosts a AppListModelView and passes AppListModel to it for display.
class AURA_SHELL_EXPORT AppListView : public views::WidgetDelegateView,
                                      public AppListItemViewListener {
 public:
  // Takes ownership of |model| and |delegate|.
  AppListView(AppListModel* model,
              AppListViewDelegate* delegate,
              const gfx::Rect& bounds,
              const ShellDelegate::SetWidgetCallback& callback);
  virtual ~AppListView();

  // Closes app list.
  void Close();

 private:
  // Initializes the window.
  void Init(const gfx::Rect& bounds,
            const ShellDelegate::SetWidgetCallback& callback);

  // Overridden from views::View:
  virtual bool OnKeyPressed(const views::KeyEvent& event) OVERRIDE;
  virtual bool OnMousePressed(const views::MouseEvent& event) OVERRIDE;

  // Overridden from AppListItemModelViewListener:
  virtual void AppListItemActivated(AppListItemView* sender,
                                    int event_flags) OVERRIDE;

  scoped_ptr<AppListModel> model_;

  scoped_ptr<AppListViewDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListView);
};

}  // namespace aura_shell

#endif  // UI_AURA_SHELL_APP_LIST_APP_LIST_VIEW_H_
