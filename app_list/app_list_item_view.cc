// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/app_list_item_view.h"

#include <algorithm>

#include "base/utf_string_conversions.h"
#include "ui/app_list/app_list_item_model.h"
#include "ui/app_list/apps_grid_view.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/animation/throb_animation.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/transform_util.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace app_list {

namespace {

const int kTopBottomPadding = 10;
const int kTopPadding = 20;
const int kIconTitleSpacing = 7;

const SkColor kTitleColor = SkColorSetRGB(0x5A, 0x5A, 0x5A);
const SkColor kTitleHoverColor = SkColorSetRGB(0x3C, 0x3C, 0x3C);

const SkColor kHoverAndPushedColor = SkColorSetARGB(0x19, 0, 0, 0);
const SkColor kSelectedColor = SkColorSetARGB(0x0D, 0, 0, 0);
const SkColor kHighlightedColor = kHoverAndPushedColor;

const int kTitleFontSize = 11;
const int kLeftRightPaddingChars = 1;

// Scale to transform the icon when a drag starts.
const float kDraggingIconScale = 1.5f;

// Delay in milliseconds of when the dragging UI should be shown for mouse drag.
const int kMouseDragUIDelayInMs = 100;

const gfx::Font& GetTitleFont() {
  static gfx::Font* font = NULL;

  // Reuses current font.
  if (font)
    return *font;

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::Font title_font(rb.GetFont(ui::ResourceBundle::BaseFont).GetFontName(),
                       kTitleFontSize);
  title_font = title_font.DeriveFont(0, gfx::Font::BOLD);
  font = new gfx::Font(title_font);
  return *font;
}

}  // namespace

// static
const char AppListItemView::kViewClassName[] = "ui/app_list/AppListItemView";

AppListItemView::AppListItemView(AppsGridView* apps_grid_view,
                                 AppListItemModel* model)
    : CustomButton(apps_grid_view),
      model_(model),
      apps_grid_view_(apps_grid_view),
      icon_(new views::ImageView),
      title_(new views::Label),
      ui_state_(UI_STATE_NORMAL),
      touch_dragging_(false) {
  icon_->set_interactive(false);

  title_->SetBackgroundColor(0);
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetEnabledColor(kTitleColor);
  title_->SetFont(GetTitleFont());

  const gfx::ShadowValue kIconShadows[] = {
    gfx::ShadowValue(gfx::Point(0, 2), 2, SkColorSetARGB(0x24, 0, 0, 0)),
  };
  icon_shadows_.assign(kIconShadows, kIconShadows + arraysize(kIconShadows));

  AddChildView(icon_);
  AddChildView(title_);

  ItemIconChanged();
  ItemTitleChanged();
  model_->AddObserver(this);

  set_context_menu_controller(this);
  set_request_focus_on_press(false);
}

AppListItemView::~AppListItemView() {
  model_->RemoveObserver(this);
}

void AppListItemView::SetIconSize(const gfx::Size& size) {
  if (icon_size_ == size)
    return;

  icon_size_ = size;
  UpdateIcon();
}

void AppListItemView::UpdateIcon() {
  // Skip if |icon_size_| has not been determined.
  if (icon_size_.IsEmpty())
    return;

  gfx::ImageSkia icon = model_->icon();
  // Clear icon and bail out if model icon is empty.
  if (icon.isNull()) {
    icon_->SetImage(NULL);
    return;
  }

  gfx::ImageSkia resized(gfx::ImageSkiaOperations::CreateResizedImage(icon,
      skia::ImageOperations::RESIZE_BEST, icon_size_));
  gfx::ImageSkia shadow(
      gfx::ImageSkiaOperations::CreateImageWithDropShadow(resized,
                                                          icon_shadows_));
  icon_->SetImage(shadow);
}

void AppListItemView::SetUIState(UIState state) {
  if (ui_state_ == state)
    return;

  ui_state_ = state;

#if !defined(OS_WIN)
  ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
  switch (ui_state_) {
    case UI_STATE_NORMAL:
      title_->SetVisible(true);
      layer()->SetTransform(gfx::Transform());
      break;
    case UI_STATE_DRAGGING:
      title_->SetVisible(false);
      const gfx::Rect bounds(layer()->bounds().size());
      layer()->SetTransform(gfx::GetScaleTransform(
          bounds.CenterPoint(),
          kDraggingIconScale));
      break;
  }
#endif
}

void AppListItemView::SetTouchDragging(bool touch_dragging) {
  if (touch_dragging_ == touch_dragging)
    return;

  touch_dragging_ = touch_dragging;
  SetUIState(touch_dragging_ ? UI_STATE_DRAGGING : UI_STATE_NORMAL);
}

void AppListItemView::OnMouseDragTimer() {
  DCHECK(apps_grid_view_->IsDraggedView(this));
  SetUIState(UI_STATE_DRAGGING);
}

void AppListItemView::ItemIconChanged() {
  UpdateIcon();
}

void AppListItemView::ItemTitleChanged() {
  title_->SetText(UTF8ToUTF16(model_->title()));
}

void AppListItemView::ItemHighlightedChanged() {
  apps_grid_view_->EnsureViewVisible(this);
  SchedulePaint();
}

std::string AppListItemView::GetClassName() const {
  return kViewClassName;
}

void AppListItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());

  int left_right_padding = kLeftRightPaddingChars *
      title_->font().GetAverageCharacterWidth();
  gfx::Size title_size = title_->GetPreferredSize();

  rect.Inset(left_right_padding, kTopPadding);
  int y = rect.y();

  gfx::Rect icon_bounds(rect.x(), y, rect.width(), icon_size_.height());
  icon_bounds.Inset(gfx::ShadowValue::GetMargin(icon_shadows_));
  icon_->SetBoundsRect(icon_bounds);

  title_->SetBounds(rect.x(),
                    y + icon_size_.height() + kIconTitleSpacing,
                    rect.width(),
                    title_size.height());
}

void AppListItemView::OnPaint(gfx::Canvas* canvas) {
  if (apps_grid_view_->IsDraggedView(this))
    return;

  gfx::Rect rect(GetContentsBounds());

  if (model_->highlighted()) {
    canvas->FillRect(rect, kHighlightedColor);
  } else if (hover_animation_->is_animating()) {
    int alpha = SkColorGetA(kHoverAndPushedColor) *
        hover_animation_->GetCurrentValue();
    canvas->FillRect(rect, SkColorSetA(kHoverAndPushedColor, alpha));
  } else if (state() == BS_HOT || state() == BS_PUSHED) {
    canvas->FillRect(rect, kHoverAndPushedColor);
  } else if (apps_grid_view_->IsSelectedView(this)) {
    canvas->FillRect(rect, kSelectedColor);
  }
}

void AppListItemView::GetAccessibleState(ui::AccessibleViewState* state) {
  state->role = ui::AccessibilityTypes::ROLE_PUSHBUTTON;
  state->name = UTF8ToUTF16(model_->title());
}

void AppListItemView::ShowContextMenuForView(views::View* source,
                                             const gfx::Point& point) {
  ui::MenuModel* menu_model = model_->GetContextMenuModel();
  if (!menu_model)
    return;

  views::MenuModelAdapter menu_adapter(menu_model);
  context_menu_runner_.reset(
      new views::MenuRunner(new views::MenuItemView(&menu_adapter)));
  menu_adapter.BuildMenu(context_menu_runner_->GetMenu());
  if (context_menu_runner_->RunMenuAt(
          GetWidget(), NULL, gfx::Rect(point, gfx::Size()),
          views::MenuItemView::TOPLEFT, views::MenuRunner::HAS_MNEMONICS) ==
      views::MenuRunner::MENU_DELETED)
    return;
}

void AppListItemView::StateChanged() {
  if (state() == BS_HOT || state() == BS_PUSHED) {
    apps_grid_view_->SetSelectedView(this);
    title_->SetEnabledColor(kTitleHoverColor);
  } else {
    apps_grid_view_->ClearSelectedView(this);
    model_->SetHighlighted(false);
    title_->SetEnabledColor(kTitleColor);
  }
}

bool AppListItemView::ShouldEnterPushedState(const ui::Event& event) {
  // Don't enter pushed state for ET_GESTURE_TAP_DOWN so that hover gray
  // background does not show up during scroll.
  if (event.type() == ui::ET_GESTURE_TAP_DOWN)
    return false;

  return views::CustomButton::ShouldEnterPushedState(event);
}

bool AppListItemView::OnMousePressed(const ui::MouseEvent& event) {
  CustomButton::OnMousePressed(event);
  apps_grid_view_->InitiateDrag(this, AppsGridView::MOUSE, event);

  if (apps_grid_view_->IsDraggedView(this)) {
    mouse_drag_timer_.Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(kMouseDragUIDelayInMs),
        this, &AppListItemView::OnMouseDragTimer);
  }
  return true;
}

void AppListItemView::OnMouseReleased(const ui::MouseEvent& event) {
  CustomButton::OnMouseReleased(event);
  apps_grid_view_->EndDrag(false);
  mouse_drag_timer_.Stop();
  SetUIState(UI_STATE_NORMAL);
}

void AppListItemView::OnMouseCaptureLost() {
  CustomButton::OnMouseCaptureLost();
  apps_grid_view_->EndDrag(true);
  mouse_drag_timer_.Stop();
  SetUIState(UI_STATE_NORMAL);
}

bool AppListItemView::OnMouseDragged(const ui::MouseEvent& event) {
  CustomButton::OnMouseDragged(event);
  apps_grid_view_->UpdateDrag(this, AppsGridView::MOUSE, event);

  // Shows dragging UI when it's confirmed without waiting for the timer.
  if (ui_state_ != UI_STATE_DRAGGING &&
      apps_grid_view_->dragging() &&
      apps_grid_view_->IsDraggedView(this)) {
    mouse_drag_timer_.Stop();
    SetUIState(UI_STATE_DRAGGING);
  }
  return true;
}

ui::EventResult AppListItemView::OnGestureEvent(
    const ui::GestureEvent& event) {
  switch (event.type()) {
    case ui::ET_GESTURE_SCROLL_BEGIN:
      if (touch_dragging_) {
        apps_grid_view_->InitiateDrag(this, AppsGridView::TOUCH, event);
        return ui::ER_CONSUMED;
      }
      break;
    case ui::ET_GESTURE_SCROLL_UPDATE:
      if (touch_dragging_) {
        apps_grid_view_->UpdateDrag(this, AppsGridView::TOUCH, event);
        return ui::ER_CONSUMED;
      }
      break;
    case ui::ET_GESTURE_SCROLL_END:
    case ui::ET_SCROLL_FLING_START:
      if (touch_dragging_) {
        SetTouchDragging(false);
        apps_grid_view_->EndDrag(false);
        return ui::ER_CONSUMED;
      }
      break;
    case ui::ET_GESTURE_LONG_PRESS:
      if (!apps_grid_view_->has_dragged_view())
        SetTouchDragging(true);
      return ui::ER_CONSUMED;
    case ui::ET_GESTURE_END:
      if (touch_dragging_) {
        SetTouchDragging(false);

        gfx::Point location(event.location());
        ConvertPointToScreen(this, &location);
        ShowContextMenu(location, true);
      }
      break;
    default:
      break;
  }
  return CustomButton::OnGestureEvent(event);
}

}  // namespace app_list
