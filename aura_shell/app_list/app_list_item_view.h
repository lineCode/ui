// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SHELL_APP_LIST_APP_LIST_ITEM_VIEW_H_
#define UI_AURA_SHELL_APP_LIST_APP_LIST_ITEM_VIEW_H_
#pragma once

#include "ui/aura_shell/app_list/app_list_item_model_observer.h"
#include "ui/aura_shell/aura_shell_export.h"
#include "ui/views/view.h"

class SkBitmap;

namespace views {
class ImageView;
class Label;
}

namespace aura_shell {

class AppListItemModel;
class AppListItemViewListener;

class AURA_SHELL_EXPORT AppListItemView : public views::View,
                                          public AppListItemModelObserver {
 public:
  AppListItemView(AppListItemModel* model,
                  AppListItemViewListener* listener);
  virtual ~AppListItemView();

  AppListItemModel* model() const {
    return model_;
  }

  // Tile size
  static const int kTileSize = 180;

  // Preferred icon size.
  static const int kIconSize = 128;

 protected:
  // Notifies listener when activated.
  void NotifyActivated(int event_flags);

  // AppListItemModelObserver overrides:
  virtual void ItemIconChanged() OVERRIDE;
  virtual void ItemTitleChanged() OVERRIDE;

  // views::View overrides:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual void OnFocus() OVERRIDE;
  virtual void OnBlur() OVERRIDE;
  virtual bool OnKeyPressed(const views::KeyEvent& event) OVERRIDE;
  virtual bool OnMousePressed(const views::MouseEvent& event) OVERRIDE;
  virtual void OnMouseReleased(const views::MouseEvent& event) OVERRIDE;
  virtual void OnPaintFocusBorder(gfx::Canvas* canvas) OVERRIDE;

 private:
  AppListItemModel* model_;
  AppListItemViewListener* listener_;

  views::ImageView* icon_;
  views::Label* title_;

  DISALLOW_COPY_AND_ASSIGN(AppListItemView);
};

}  // namespace aura_shell

#endif  // UI_AURA_SHELL_APP_LIST_APP_LIST_ITEM_VIEW_H_
