// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_ITEM_VIEW_H_
#define UI_APP_LIST_APP_LIST_ITEM_VIEW_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/app_list_item_model_observer.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/custom_button.h"

class SkBitmap;

namespace views {
class ImageView;
class Label;
class MenuRunner;
}

namespace app_list {

class AppListItemModel;
class AppsGridView;

class APP_LIST_EXPORT AppListItemView : public views::CustomButton,
                                        public views::ContextMenuController,
                                        public AppListItemModelObserver {
 public:
  // Internal class name.
  static const char kViewClassName[];

  AppListItemView(AppsGridView* apps_grid_view, AppListItemModel* model);
  virtual ~AppListItemView();

  void SetIconSize(const gfx::Size& size);

  AppListItemModel* model() const { return model_; }

 private:
  // Get icon from model and schedule background processing.
  void UpdateIcon();

  // AppListItemModelObserver overrides:
  virtual void ItemIconChanged() OVERRIDE;
  virtual void ItemTitleChanged() OVERRIDE;
  virtual void ItemHighlightedChanged() OVERRIDE;

  // views::View overrides:
  virtual std::string GetClassName() const OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

  // views::ContextMenuController overrides:
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) OVERRIDE;

  // views::CustomButton overrides:
  virtual void StateChanged() OVERRIDE;
  virtual bool ShouldEnterPushedState(const ui::Event& event) OVERRIDE;

  // views::View overrides:
  virtual bool OnMousePressed(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseReleased(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseCaptureLost() OVERRIDE;
  virtual bool OnMouseDragged(const ui::MouseEvent& event) OVERRIDE;

  AppListItemModel* model_;  // Owned by AppListModel::Apps.

  AppsGridView* apps_grid_view_;  // Owned by views hierarchy.
  views::ImageView* icon_;  // Owned by views hierarchy.
  views::Label* title_;  // Owned by views hierarchy.

  scoped_ptr<views::MenuRunner> context_menu_runner_;

  gfx::Size icon_size_;
  gfx::ShadowValues icon_shadows_;

  DISALLOW_COPY_AND_ASSIGN(AppListItemView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_ITEM_VIEW_H_
