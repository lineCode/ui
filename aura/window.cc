// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window.h"

#include <algorithm>

#include "base/logging.h"
#include "ui/aura/desktop.h"
#include "ui/aura/event.h"
#include "ui/aura/event_filter.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window_delegate.h"
#include "ui/gfx/canvas_skia.h"
#include "ui/gfx/compositor/compositor.h"
#include "ui/gfx/compositor/layer.h"

namespace aura {

Window::Window(WindowDelegate* delegate)
    : delegate_(delegate),
      visibility_(VISIBILITY_HIDDEN),
      parent_(NULL),
      id_(-1),
      user_data_(NULL) {
}

Window::~Window() {
  // Let the delegate know we're in the processing of destroying.
  if (delegate_)
    delegate_->OnWindowDestroying();

  // Update the FocusManager in case we were focused. This must be done before
  // we are removed from the hierarchy otherwise we won't be able to find the
  // FocusManager.
  internal::FocusManager* focus_manager = GetFocusManager();
  if (focus_manager && focus_manager->focused_window() == this)
    focus_manager->SetFocusedWindow(NULL);

  // Then destroy the children.
  while (!children_.empty()) {
    Window* child = children_[0];
    delete child;
    // Deleting the child so remove it from out children_ list.
    DCHECK(std::find(children_.begin(), children_.end(), child) ==
           children_.end());
  }
  // And let the delegate do any post cleanup.
  if (delegate_)
    delegate_->OnWindowDestroyed();
  if (parent_)
    parent_->RemoveChild(this);
}

void Window::Init() {
  ui::Layer::TextureParam param = ui::Layer::LAYER_HAS_NO_TEXTURE;
  if (delegate_)
    param = ui::Layer::LAYER_HAS_TEXTURE;
  layer_.reset(new ui::Layer(Desktop::GetInstance()->compositor(), param));
  layer_->set_delegate(this);
}

void Window::SetVisibility(Visibility visibility) {
  if (visibility_ == visibility)
    return;

  visibility_ = visibility;
  layer_->set_visible(visibility_ != VISIBILITY_HIDDEN);
  if (layer_->visible())
    SchedulePaint();
}

void Window::SetLayoutManager(LayoutManager* layout_manager) {
  layout_manager_.reset(layout_manager);
}

void Window::SetBounds(const gfx::Rect& bounds, int anim_ms) {
  // TODO: support anim_ms
  // TODO: funnel this through the Desktop.
  bool was_move = bounds_.size() == bounds.size();
  gfx::Rect old_bounds = bounds_;
  bounds_ = bounds;
  layer_->SetBounds(bounds);
  if (layout_manager_.get())
    layout_manager_->OnWindowResized();
  if (delegate_)
    delegate_->OnBoundsChanged(old_bounds, bounds_);
  if (was_move)
    SchedulePaintInRect(gfx::Rect());
  else
    SchedulePaint();
}

void Window::SchedulePaintInRect(const gfx::Rect& rect) {
  layer_->SchedulePaint(rect);
}

void Window::SetCanvas(const SkCanvas& canvas, const gfx::Point& origin) {
  // TODO: figure out how this is going to work when animating the layer. In
  // particular if we're animating the size then the underlying Texture is going
  // to be unhappy if we try to set a texture on a size bigger than the size of
  // the texture.
  layer_->SetCanvas(canvas, origin);
}

void Window::SetParent(Window* parent) {
  if (parent)
    parent->AddChild(this);
  else
    Desktop::GetInstance()->toplevel_window_container()->AddChild(this);
}

bool Window::IsToplevelWindowContainer() const {
  return false;
}

void Window::MoveChildToFront(Window* child) {
  DCHECK_EQ(child->parent(), this);
  const Windows::iterator i(std::find(children_.begin(), children_.end(),
                                      child));
  DCHECK(i != children_.end());
  children_.erase(i);

  // TODO(beng): this obviously has to handle different window types.
  children_.insert(children_.begin() + children_.size(), child);
  SchedulePaintInRect(gfx::Rect());
}

void Window::AddChild(Window* child) {
  DCHECK(std::find(children_.begin(), children_.end(), child) ==
      children_.end());
  child->parent_ = this;
  layer_->Add(child->layer_.get());
  children_.push_back(child);
}

void Window::RemoveChild(Window* child) {
  Windows::iterator i = std::find(children_.begin(), children_.end(), child);
  DCHECK(i != children_.end());
  child->parent_ = NULL;
  layer_->Remove(child->layer_.get());
  children_.erase(i);
}

// static
void Window::ConvertPointToWindow(Window* source,
                                  Window* target,
                                  gfx::Point* point) {
  ui::Layer::ConvertPointToLayer(source->layer(), target->layer(), point);
}

void Window::SetEventFilter(EventFilter* event_filter) {
  event_filter_.reset(event_filter);
}

bool Window::OnMouseEvent(MouseEvent* event) {
  if (!parent_)
    return false;
  if (!parent_->event_filter_.get())
    parent_->SetEventFilter(new EventFilter(parent_));
  return parent_->event_filter_->OnMouseEvent(this, event) ||
      delegate_->OnMouseEvent(event);
}

bool Window::OnKeyEvent(KeyEvent* event) {
  return delegate_->OnKeyEvent(event);
}

bool Window::HitTest(const gfx::Point& point) {
  gfx::Rect local_bounds(gfx::Point(), bounds().size());
  // TODO(beng): hittest masks.
  return local_bounds.Contains(point);
}

Window* Window::GetEventHandlerForPoint(const gfx::Point& point) {
  Windows::const_reverse_iterator i = children_.rbegin();
  for (; i != children_.rend(); ++i) {
    Window* child = *i;
    if (child->visibility() == Window::VISIBILITY_HIDDEN)
      continue;
    gfx::Point point_in_child_coords(point);
    Window::ConvertPointToWindow(this, child, &point_in_child_coords);
    if (child->HitTest(point_in_child_coords)) {
      Window* handler = child->GetEventHandlerForPoint(point_in_child_coords);
      if (handler)
        return handler;
    }
  }
  return delegate_ ? this : NULL;
}

internal::FocusManager* Window::GetFocusManager() {
  return parent_ ? parent_->GetFocusManager() : NULL;
}

void Window::SchedulePaint() {
  SchedulePaintInRect(gfx::Rect(0, 0, bounds_.width(), bounds_.height()));
}

void Window::OnPaintLayer(gfx::Canvas* canvas) {
  delegate_->OnPaint(canvas);
}

}  // namespace aura
