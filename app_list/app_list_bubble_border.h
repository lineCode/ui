// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_BUBBLE_BORDER_H_
#define UI_APP_LIST_APP_LIST_BUBBLE_BORDER_H_
#pragma once

#include "base/basictypes.h"
#include "ui/views/bubble/bubble_border.h"

namespace app_list {

// A class to paint bubble border and background.
class AppListBubbleBorder : public views::BubbleBorder {
 public:
  AppListBubbleBorder(views::View* app_list_view,
                      views::View* search_box_view,
                      views::View* grid_view,
                      views::View* results_view);
  virtual ~AppListBubbleBorder();

  int arrow_offset() const { return arrow_offset_; }
  void set_arrow_offset(int arrow_offset) { arrow_offset_ = arrow_offset; }

 private:
  void PaintSearchBoxBackground(gfx::Canvas* canvas,
                                const gfx::Rect& bounds) const;
  void PaintSearchResultListBackground(gfx::Canvas* canvas,
                                       const gfx::Rect& bounds) const;
  void PaintAppsGridBackground(gfx::Canvas* canvas,
                               const gfx::Rect& bounds) const;

  // views::BubbleBorder overrides:
  virtual void GetInsets(gfx::Insets* insets) const OVERRIDE;
  virtual gfx::Rect GetBounds(const gfx::Rect& position_relative_to,
                              const gfx::Size& contents_size) const OVERRIDE;

  // views::Border overrides:
  virtual void Paint(const views::View& view,
                     gfx::Canvas* canvas) const OVERRIDE;

  // AppListView hosted inside this bubble.
  const views::View* app_list_view_;

  // Children view of AppListView that needs to paint background.
  const views::View* search_box_view_;
  const views::View* grid_view_;
  const views::View* results_view_;

  // Offset in pixels relative the default middle position.
  int arrow_offset_;

  DISALLOW_COPY_AND_ASSIGN(AppListBubbleBorder);
};

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_BUBBLE_BORDER_H_
