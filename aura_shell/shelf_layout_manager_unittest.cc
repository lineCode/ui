// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura_shell/shelf_layout_manager.h"

#include "ui/aura/root_window.h"
#include "ui/aura/screen_aura.h"
#include "ui/aura/window.h"
#include "ui/aura_shell/shell.h"
#include "ui/aura_shell/shell_window_ids.h"
#include "ui/aura_shell/test/aura_shell_test_base.h"
#include "ui/base/animation/animation_container_element.h"
#include "ui/gfx/compositor/layer_animator.h"
#include "ui/gfx/compositor/layer.h"
#include "ui/views/widget/widget.h"

namespace aura_shell {
namespace internal {

namespace {

void StepWidgetLayerAnimatorToEnd(views::Widget* widget) {
  ui::AnimationContainerElement* element =
      static_cast<ui::AnimationContainerElement*>(
      widget->GetNativeView()->layer()->GetAnimator());
  element->Step(base::TimeTicks::Now() + base::TimeDelta::FromSeconds(1));
}

ShelfLayoutManager* GetShelfLayoutManager() {
  aura::Window* window = aura_shell::Shell::GetInstance()->GetContainer(
      aura_shell::internal::kShellWindowId_LauncherContainer);
  return static_cast<ShelfLayoutManager*>(window->layout_manager());
}

}  // namespace

typedef aura_shell::test::AuraShellTestBase ShelfLayoutManagerTest;

// Makes sure SetVisible updates work area and widget appropriately.
TEST_F(ShelfLayoutManagerTest, SetVisible) {
  ShelfLayoutManager* shelf = GetShelfLayoutManager();
  // Force an initial layout.
  shelf->LayoutShelf();
  ASSERT_TRUE(shelf->visible());

  aura::ScreenAura* screen = aura::RootWindow::GetInstance()->screen();
  ASSERT_TRUE(screen);
  // Bottom inset should be the max of widget heights.
  EXPECT_EQ(shelf->max_height(), screen->work_area_insets().bottom());

  // Hide the shelf.
  shelf->SetVisible(false);
  // Run the animation to completion.
  StepWidgetLayerAnimatorToEnd(shelf->launcher());
  StepWidgetLayerAnimatorToEnd(shelf->status());
  EXPECT_FALSE(shelf->visible());
  EXPECT_EQ(0, screen->work_area_insets().bottom());

  // Make sure the bounds of the two widgets changed.
  EXPECT_GE(shelf->launcher()->GetNativeView()->bounds().y(),
            gfx::Screen::GetPrimaryMonitorBounds().bottom());
  EXPECT_GE(shelf->status()->GetNativeView()->bounds().y(),
            gfx::Screen::GetPrimaryMonitorBounds().bottom());

  // And show it again.
  shelf->SetVisible(true);
  // Run the animation to completion.
  StepWidgetLayerAnimatorToEnd(shelf->launcher());
  StepWidgetLayerAnimatorToEnd(shelf->status());
  EXPECT_TRUE(shelf->visible());
  EXPECT_EQ(shelf->max_height(), screen->work_area_insets().bottom());

  // Make sure the bounds of the two widgets changed.
  gfx::Rect launcher_bounds(shelf->launcher()->GetNativeView()->bounds());
  int bottom = gfx::Screen::GetPrimaryMonitorBounds().bottom() -
      shelf->max_height();
  EXPECT_EQ(launcher_bounds.y(),
            bottom + (shelf->max_height() - launcher_bounds.height()) / 2);
  gfx::Rect status_bounds(shelf->status()->GetNativeView()->bounds());
  EXPECT_EQ(status_bounds.y(),
            bottom + (shelf->max_height() - status_bounds.height()) / 2);
}

// Makes sure LayoutShelf invoked while animating cleans things up.
TEST_F(ShelfLayoutManagerTest, LayoutShelfWhileAnimating) {
  ShelfLayoutManager* shelf = GetShelfLayoutManager();
  // Force an initial layout.
  shelf->LayoutShelf();
  ASSERT_TRUE(shelf->visible());

  aura::ScreenAura* screen = aura::RootWindow::GetInstance()->screen();

  // Hide the shelf.
  shelf->SetVisible(false);
  shelf->LayoutShelf();
  EXPECT_FALSE(shelf->visible());
  EXPECT_FALSE(shelf->visible());
  EXPECT_EQ(0, screen->work_area_insets().bottom());
  // Make sure the bounds of the two widgets changed.
  EXPECT_GE(shelf->launcher()->GetNativeView()->bounds().y(),
            gfx::Screen::GetPrimaryMonitorBounds().bottom());
  EXPECT_GE(shelf->status()->GetNativeView()->bounds().y(),
            gfx::Screen::GetPrimaryMonitorBounds().bottom());
}

}  // namespace internal
}  // namespace aura_shell
