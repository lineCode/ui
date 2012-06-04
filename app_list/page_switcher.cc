// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/page_switcher.h"

#include "third_party/skia/include/core/SkPath.h"
#include "ui/app_list/pagination_model.h"
#include "ui/base/animation/throb_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/controls/button/custom_button.h"
#include "ui/views/layout/box_layout.h"

namespace {

const int kPreferredHeight = 57;

const int kButtonSpacing = 18;
const int kButtonWidth = 68;
const int kButtonHeight = 6;
const int kButtonCornerRadius = 2;

const SkColor kHoverColor = SkColorSetRGB(0xB4, 0xB4, 0xB4);

const SkColor kNormalColor = SkColorSetRGB(0xE2, 0xE2, 0xE2);

const SkColor kSelectedColor = SkColorSetRGB(0x46, 0x8F, 0xFC);

class PageSwitcherButton : public views::CustomButton {
 public:
  explicit PageSwitcherButton(views::ButtonListener* listener)
      : views::CustomButton(listener),
        selected_(false) {
  }
  virtual ~PageSwitcherButton() {}

  void SetSelected(bool selected) {
    if (selected == selected_)
      return;

    selected_ = selected;
    SchedulePaint();
  }

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE {
    return gfx::Size(kButtonWidth, kButtonHeight);
  }

  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE {
    if (selected_ || state() == BS_PUSHED) {
      PaintButton(canvas, kSelectedColor);
    } else if (state() == BS_HOT) {
      PaintButton(canvas, kHoverColor);
    } else {
      PaintButton(canvas, kNormalColor);
    }
  }

 private:
  // Paints a button that has two rounded corner at bottom.
  void PaintButton(gfx::Canvas* canvas, SkColor color) {
    gfx::Rect rect(GetContentsBounds().Center(
            gfx::Size(kButtonWidth, kButtonHeight)));

    SkPath path;
    path.addRoundRect(gfx::RectToSkRect(rect),
                      SkIntToScalar(kButtonCornerRadius),
                      SkIntToScalar(kButtonCornerRadius));

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(color);
    canvas->DrawPath(path, paint);
  }

  bool selected_;

  DISALLOW_COPY_AND_ASSIGN(PageSwitcherButton);
};

// Gets PageSwitcherButton at |index| in |buttons|.
PageSwitcherButton* GetButtonByIndex(views::View* buttons, int index) {
  return static_cast<PageSwitcherButton*>(buttons->child_at(index));
}

}  // namespace

namespace app_list {

PageSwitcher::PageSwitcher(PaginationModel* model)
    : model_(model),
      buttons_(NULL) {
  buttons_ = new views::View;
  buttons_->SetLayoutManager(new views::BoxLayout(
      views::BoxLayout::kHorizontal, 0, 0, kButtonSpacing));
  AddChildView(buttons_);

  TotalPagesChanged();
  SelectedPageChanged(-1, model->selected_page());
  model_->AddObserver(this);
}

PageSwitcher::~PageSwitcher() {
  model_->RemoveObserver(this);
}

gfx::Size PageSwitcher::GetPreferredSize() {
  // Always return a size with correct height so that container resize is not
  // needed when more pages are added.
  return gfx::Size(buttons_->GetPreferredSize().width(),
                   kPreferredHeight);
}

void PageSwitcher::Layout() {
  gfx::Rect rect(GetContentsBounds());
  // Makes |buttons_| horizontally center and vertically fill.
  gfx::Size buttons_size(buttons_->GetPreferredSize());
  gfx::Rect buttons_bounds(rect.CenterPoint().x() - buttons_size.width() / 2,
                           rect.y(),
                           buttons_size.width(),
                           rect.height());
  buttons_->SetBoundsRect(rect.Intersect(buttons_bounds));
}

void PageSwitcher::ButtonPressed(views::Button* sender,
                                 const views::Event& event) {
  for (int i = 0; i < buttons_->child_count(); ++i) {
    if (sender == static_cast<views::Button*>(buttons_->child_at(i))) {
      model_->SelectPage(i);
      break;
    }
  }
}

void PageSwitcher::TotalPagesChanged() {
  buttons_->RemoveAllChildViews(true);
  for (int i = 0; i < model_->total_pages(); ++i) {
    PageSwitcherButton* button = new PageSwitcherButton(this);
    button->SetSelected(i == model_->selected_page());
    buttons_->AddChildView(button);
  }
  buttons_->SetVisible(model_->total_pages() > 1);
  Layout();
}

void PageSwitcher::SelectedPageChanged(int old_selected, int new_selected) {
  if (old_selected >= 0 && old_selected < buttons_->child_count())
    GetButtonByIndex(buttons_, old_selected)->SetSelected(false);
  if (new_selected >= 0 && new_selected < buttons_->child_count())
    GetButtonByIndex(buttons_, new_selected)->SetSelected(true);
}

}  // namespace app_list
