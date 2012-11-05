// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_native_widget_aura.h"

#include "base/bind.h"
#include "ui/aura/client/stacking_client.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/root_window.h"
#include "ui/aura/root_window_host.h"
#include "ui/aura/window.h"
#include "ui/aura/window_property.h"
#include "ui/base/hit_test.h"
#include "ui/base/native_theme/native_theme.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/views/ime/input_method.h"
#include "ui/views/widget/desktop_root_window_host.h"
#include "ui/views/widget/native_widget_aura_window_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_aura_utils.h"

DECLARE_EXPORTED_WINDOW_PROPERTY_TYPE(VIEWS_EXPORT,
                                      views::DesktopNativeWidgetAura*);

namespace views {

DEFINE_WINDOW_PROPERTY_KEY(DesktopNativeWidgetAura*,
                           kDesktopNativeWidgetAuraKey, NULL);

namespace {

class DesktopNativeWidgetAuraStackingClient :
    public aura::client::StackingClient {
 public:
  explicit DesktopNativeWidgetAuraStackingClient(aura::RootWindow* root_window)
      : root_window_(root_window) {
    aura::client::SetStackingClient(root_window_, this);
  }
  virtual ~DesktopNativeWidgetAuraStackingClient() {
    aura::client::SetStackingClient(root_window_, NULL);
  }

  // Overridden from client::StackingClient:
  virtual aura::Window* GetDefaultParent(aura::Window* window,
                                         const gfx::Rect& bounds) OVERRIDE {
    return root_window_;
  }

 private:
  aura::RootWindow* root_window_;

  DISALLOW_COPY_AND_ASSIGN(DesktopNativeWidgetAuraStackingClient);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, public:

DesktopNativeWidgetAura::DesktopNativeWidgetAura(
    internal::NativeWidgetDelegate* delegate)
    : ownership_(Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET),
      ALLOW_THIS_IN_INITIALIZER_LIST(close_widget_factory_(this)),
      can_activate_(true),
      desktop_root_window_host_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(window_(new aura::Window(this))),
      native_widget_delegate_(delegate) {
  window_->SetProperty(kDesktopNativeWidgetAuraKey, this);
}

DesktopNativeWidgetAura::~DesktopNativeWidgetAura() {
  if (ownership_ == Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete native_widget_delegate_;
  else
    CloseNow();
}

// static
DesktopNativeWidgetAura* DesktopNativeWidgetAura::ForWindow(
    aura::Window* window) {
  return window->GetProperty(kDesktopNativeWidgetAuraKey);
}

void DesktopNativeWidgetAura::OnHostClosed() {
  // This will, through a long list of callbacks, trigger |root_window_| going
  // away. See OnWindowDestroyed()
  delete window_;
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, internal::NativeWidgetPrivate implementation:

void DesktopNativeWidgetAura::InitNativeWidget(
    const Widget::InitParams& params) {
  ownership_ = params.ownership;

  window_->set_user_data(this);
  window_->SetType(GetAuraWindowTypeForWidgetType(params.type));
  window_->SetTransparent(true);
  window_->Init(params.layer_type);
  window_->Show();

  desktop_root_window_host_ = params.desktop_root_window_host ?
      params.desktop_root_window_host :
      DesktopRootWindowHost::Create(native_widget_delegate_,
                                    this, params.bounds);
  root_window_.reset(
      desktop_root_window_host_->Init(window_, params));
  stacking_client_.reset(
      new DesktopNativeWidgetAuraStackingClient(root_window_.get()));

  aura::client::SetActivationDelegate(window_, this);
}

NonClientFrameView* DesktopNativeWidgetAura::CreateNonClientFrameView() {
  return desktop_root_window_host_->CreateNonClientFrameView();
}

bool DesktopNativeWidgetAura::ShouldUseNativeFrame() const {
  return desktop_root_window_host_->ShouldUseNativeFrame();
}

void DesktopNativeWidgetAura::FrameTypeChanged() {
  desktop_root_window_host_->FrameTypeChanged();
}

Widget* DesktopNativeWidgetAura::GetWidget() {
  return native_widget_delegate_->AsWidget();
}

const Widget* DesktopNativeWidgetAura::GetWidget() const {
  return native_widget_delegate_->AsWidget();
}

gfx::NativeView DesktopNativeWidgetAura::GetNativeView() const {
  return window_;
}

gfx::NativeWindow DesktopNativeWidgetAura::GetNativeWindow() const {
  return window_;
}

Widget* DesktopNativeWidgetAura::GetTopLevelWidget() {
  return GetWidget();
}

const ui::Compositor* DesktopNativeWidgetAura::GetCompositor() const {
  return window_->layer()->GetCompositor();
}

ui::Compositor* DesktopNativeWidgetAura::GetCompositor() {
  return window_->layer()->GetCompositor();
}

void DesktopNativeWidgetAura::CalculateOffsetToAncestorWithLayer(
      gfx::Point* offset,
      ui::Layer** layer_parent) {
  if (layer_parent)
    *layer_parent = window_->layer();
}

void DesktopNativeWidgetAura::ViewRemoved(View* view) {
}

void DesktopNativeWidgetAura::SetNativeWindowProperty(const char* name,
                                                      void* value) {
  window_->SetNativeWindowProperty(name, value);
}

void* DesktopNativeWidgetAura::GetNativeWindowProperty(const char* name) const {
  return window_->GetNativeWindowProperty(name);
}

TooltipManager* DesktopNativeWidgetAura::GetTooltipManager() const {
  return NULL;
}

bool DesktopNativeWidgetAura::IsScreenReaderActive() const {
  return false;
}

void DesktopNativeWidgetAura::SendNativeAccessibilityEvent(
      View* view,
      ui::AccessibilityTypes::Event event_type) {
}

void DesktopNativeWidgetAura::SetCapture() {
  window_->SetCapture();
  // aura::Window doesn't implicitly update capture on the RootWindowHost, so
  // we have to do that manually.
  if (!desktop_root_window_host_->HasCapture())
    window_->GetRootWindow()->SetNativeCapture();
}

void DesktopNativeWidgetAura::ReleaseCapture() {
  window_->ReleaseCapture();
  // aura::Window doesn't implicitly update capture on the RootWindowHost, so
  // we have to do that manually.
  if (desktop_root_window_host_->HasCapture())
    window_->GetRootWindow()->ReleaseNativeCapture();
}

bool DesktopNativeWidgetAura::HasCapture() const {
  return window_->HasCapture() && desktop_root_window_host_->HasCapture();
}

InputMethod* DesktopNativeWidgetAura::CreateInputMethod() {
  return desktop_root_window_host_->CreateInputMethod();
}

internal::InputMethodDelegate*
    DesktopNativeWidgetAura::GetInputMethodDelegate() {
  return desktop_root_window_host_->GetInputMethodDelegate();
}

void DesktopNativeWidgetAura::CenterWindow(const gfx::Size& size) {
  desktop_root_window_host_->CenterWindow(size);
}

void DesktopNativeWidgetAura::GetWindowPlacement(
      gfx::Rect* bounds,
      ui::WindowShowState* maximized) const {
  desktop_root_window_host_->GetWindowPlacement(bounds, maximized);
}

void DesktopNativeWidgetAura::SetWindowTitle(const string16& title) {
  desktop_root_window_host_->SetWindowTitle(title);
}

void DesktopNativeWidgetAura::SetWindowIcons(const gfx::ImageSkia& window_icon,
                                             const gfx::ImageSkia& app_icon) {
}

void DesktopNativeWidgetAura::SetAccessibleName(const string16& name) {
}

void DesktopNativeWidgetAura::SetAccessibleRole(
    ui::AccessibilityTypes::Role role) {
}

void DesktopNativeWidgetAura::SetAccessibleState(
    ui::AccessibilityTypes::State state) {
}

void DesktopNativeWidgetAura::InitModalType(ui::ModalType modal_type) {
}

gfx::Rect DesktopNativeWidgetAura::GetWindowBoundsInScreen() const {
  return desktop_root_window_host_->GetWindowBoundsInScreen();
}

gfx::Rect DesktopNativeWidgetAura::GetClientAreaBoundsInScreen() const {
  return desktop_root_window_host_->GetClientAreaBoundsInScreen();
}

gfx::Rect DesktopNativeWidgetAura::GetRestoredBounds() const {
  return desktop_root_window_host_->GetRestoredBounds();
}

void DesktopNativeWidgetAura::SetBounds(const gfx::Rect& bounds) {
  desktop_root_window_host_->AsRootWindowHost()->SetBounds(bounds);
}

void DesktopNativeWidgetAura::SetSize(const gfx::Size& size) {
  desktop_root_window_host_->SetSize(size);
}

void DesktopNativeWidgetAura::StackAbove(gfx::NativeView native_view) {
}

void DesktopNativeWidgetAura::StackAtTop() {
}

void DesktopNativeWidgetAura::StackBelow(gfx::NativeView native_view) {
}

void DesktopNativeWidgetAura::SetShape(gfx::NativeRegion shape) {
  desktop_root_window_host_->SetShape(shape);
}

void DesktopNativeWidgetAura::Close() {
  desktop_root_window_host_->Close();
}

void DesktopNativeWidgetAura::CloseNow() {
  desktop_root_window_host_->CloseNow();
}

void DesktopNativeWidgetAura::Show() {
  desktop_root_window_host_->AsRootWindowHost()->Show();
}

void DesktopNativeWidgetAura::Hide() {
  desktop_root_window_host_->AsRootWindowHost()->Hide();
}

void DesktopNativeWidgetAura::ShowMaximizedWithBounds(
      const gfx::Rect& restored_bounds) {
  desktop_root_window_host_->ShowMaximizedWithBounds(restored_bounds);
}

void DesktopNativeWidgetAura::ShowWithWindowState(ui::WindowShowState state) {
  desktop_root_window_host_->ShowWindowWithState(state);
}

bool DesktopNativeWidgetAura::IsVisible() const {
  return desktop_root_window_host_->IsVisible();
}

void DesktopNativeWidgetAura::Activate() {
  desktop_root_window_host_->Activate();
}

void DesktopNativeWidgetAura::Deactivate() {
  desktop_root_window_host_->Deactivate();
}

bool DesktopNativeWidgetAura::IsActive() const {
  return desktop_root_window_host_->IsActive();
}

void DesktopNativeWidgetAura::SetAlwaysOnTop(bool always_on_top) {
  desktop_root_window_host_->SetAlwaysOnTop(always_on_top);
}

void DesktopNativeWidgetAura::Maximize() {
  desktop_root_window_host_->Maximize();
}

void DesktopNativeWidgetAura::Minimize() {
  desktop_root_window_host_->Minimize();
}

bool DesktopNativeWidgetAura::IsMaximized() const {
  return desktop_root_window_host_->IsMaximized();
}

bool DesktopNativeWidgetAura::IsMinimized() const {
  return desktop_root_window_host_->IsMinimized();
}

void DesktopNativeWidgetAura::Restore() {
  desktop_root_window_host_->Restore();
}

void DesktopNativeWidgetAura::SetFullscreen(bool fullscreen) {
  desktop_root_window_host_->SetFullscreen(fullscreen);
}

bool DesktopNativeWidgetAura::IsFullscreen() const {
  return desktop_root_window_host_->IsFullscreen();
}

void DesktopNativeWidgetAura::SetOpacity(unsigned char opacity) {
  desktop_root_window_host_->SetOpacity(opacity);
}

void DesktopNativeWidgetAura::SetUseDragFrame(bool use_drag_frame) {
}

void DesktopNativeWidgetAura::FlashFrame(bool flash_frame) {
  desktop_root_window_host_->FlashFrame(flash_frame);
}

bool DesktopNativeWidgetAura::IsAccessibleWidget() const {
  return false;
}

void DesktopNativeWidgetAura::RunShellDrag(View* view,
                            const ui::OSExchangeData& data,
                            const gfx::Point& location,
                            int operation) {
}

void DesktopNativeWidgetAura::SchedulePaintInRect(const gfx::Rect& rect) {
  if (window_)
    window_->SchedulePaintInRect(rect);
}

void DesktopNativeWidgetAura::SetCursor(gfx::NativeCursor cursor) {
  desktop_root_window_host_->AsRootWindowHost()->SetCursor(cursor);
}

void DesktopNativeWidgetAura::ClearNativeFocus() {
  desktop_root_window_host_->ClearNativeFocus();
}

gfx::Rect DesktopNativeWidgetAura::GetWorkAreaBoundsInScreen() const {
  return desktop_root_window_host_->GetWorkAreaBoundsInScreen();
}

void DesktopNativeWidgetAura::SetInactiveRenderingDisabled(bool value) {
  if (!value) {
    active_window_observer_.reset();
  } else {
    active_window_observer_.reset(
        new NativeWidgetAuraWindowObserver(window_, native_widget_delegate_));
  }
}

Widget::MoveLoopResult DesktopNativeWidgetAura::RunMoveLoop(
      const gfx::Vector2d& drag_offset) {
  return desktop_root_window_host_->RunMoveLoop(drag_offset);
}

void DesktopNativeWidgetAura::EndMoveLoop() {
  desktop_root_window_host_->EndMoveLoop();
}

void DesktopNativeWidgetAura::SetVisibilityChangedAnimationsEnabled(
    bool value) {
  desktop_root_window_host_->SetVisibilityChangedAnimationsEnabled(value);
}

ui::NativeTheme* DesktopNativeWidgetAura::GetNativeTheme() const {
  return DesktopRootWindowHost::GetNativeTheme(window_);
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, aura::WindowDelegate implementation:

gfx::Size DesktopNativeWidgetAura::GetMinimumSize() const {
  return native_widget_delegate_->GetMinimumSize();
}

void DesktopNativeWidgetAura::OnBoundsChanged(const gfx::Rect& old_bounds,
                                              const gfx::Rect& new_bounds) {
  if (old_bounds.origin() != new_bounds.origin())
    native_widget_delegate_->OnNativeWidgetMove();
  if (old_bounds.size() != new_bounds.size())
    native_widget_delegate_->OnNativeWidgetSizeChanged(new_bounds.size());
}

void DesktopNativeWidgetAura::OnFocus(aura::Window* old_focused_window) {
  desktop_root_window_host_->OnNativeWidgetFocus();
  native_widget_delegate_->OnNativeFocus(old_focused_window);
}

void DesktopNativeWidgetAura::OnBlur() {
  if (GetWidget()->HasFocusManager())
    GetWidget()->GetFocusManager()->StoreFocusedView();
  desktop_root_window_host_->OnNativeWidgetBlur();
  native_widget_delegate_->OnNativeBlur(
      window_->GetFocusManager()->GetFocusedWindow());
}

gfx::NativeCursor DesktopNativeWidgetAura::GetCursor(const gfx::Point& point) {
  return gfx::kNullCursor;
}

int DesktopNativeWidgetAura::GetNonClientComponent(
    const gfx::Point& point) const {
  return native_widget_delegate_->GetNonClientComponent(point);
}

bool DesktopNativeWidgetAura::ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) {
  return true;
}

bool DesktopNativeWidgetAura::CanFocus() {
  return true;
}

void DesktopNativeWidgetAura::OnCaptureLost() {
  native_widget_delegate_->OnMouseCaptureLost();
}

void DesktopNativeWidgetAura::OnPaint(gfx::Canvas* canvas) {
  native_widget_delegate_->OnNativeWidgetPaint(canvas);
}

void DesktopNativeWidgetAura::OnDeviceScaleFactorChanged(
    float device_scale_factor) {
}

void DesktopNativeWidgetAura::OnWindowDestroying() {
  native_widget_delegate_->OnNativeWidgetDestroying();
}

void DesktopNativeWidgetAura::OnWindowDestroyed() {
  window_ = NULL;
  native_widget_delegate_->OnNativeWidgetDestroyed();
  // TODO(beng): I think we should only do this if widget owns native widget.
  //             Verify and if untrue remove this comment.
  delete this;
}

void DesktopNativeWidgetAura::OnWindowTargetVisibilityChanged(bool visible) {
}

bool DesktopNativeWidgetAura::HasHitTestMask() const {
  return native_widget_delegate_->HasHitTestMask();
}

void DesktopNativeWidgetAura::GetHitTestMask(gfx::Path* mask) const {
  native_widget_delegate_->GetHitTestMask(mask);
}

scoped_refptr<ui::Texture> DesktopNativeWidgetAura::CopyTexture() {
  // The layer we create doesn't have an external texture, so this should never
  // get invoked.
  NOTREACHED();
  return scoped_refptr<ui::Texture>();
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, ui::EventHandler implementation:

ui::EventResult DesktopNativeWidgetAura::OnKeyEvent(ui::KeyEvent* event) {
  if (event->is_char()) {
    // If a ui::InputMethod object is attached to the root window, character
    // events are handled inside the object and are not passed to this function.
    // If such object is not attached, character events might be sent (e.g. on
    // Windows). In this case, we just skip these.
    return ui::ER_UNHANDLED;
  }
  // Renderer may send a key event back to us if the key event wasn't handled,
  // and the window may be invisible by that time.
  if (!window_->IsVisible())
    return ui::ER_UNHANDLED;
  return native_widget_delegate_->OnKeyEvent(*event) ?
      ui::ER_HANDLED : ui::ER_UNHANDLED;;
}

ui::EventResult DesktopNativeWidgetAura::OnMouseEvent(ui::MouseEvent* event) {
  DCHECK(window_->IsVisible());
  if (event->type() == ui::ET_MOUSEWHEEL) {
    return native_widget_delegate_->OnMouseEvent(*event) ?
        ui::ER_HANDLED : ui::ER_UNHANDLED;
  }

  if (event->type() == ui::ET_SCROLL) {
    if (native_widget_delegate_->OnMouseEvent(*event))
      return ui::ER_HANDLED;

    // Convert unprocessed scroll events into wheel events.
    ui::MouseWheelEvent mwe(*static_cast<ui::ScrollEvent*>(event));
    return native_widget_delegate_->OnMouseEvent(mwe) ?
        ui::ER_HANDLED : ui::ER_UNHANDLED;
  }
  return native_widget_delegate_->OnMouseEvent(*event) ?
      ui::ER_HANDLED : ui::ER_UNHANDLED;
}

ui::EventResult DesktopNativeWidgetAura::OnTouchEvent(ui::TouchEvent* event) {
  return native_widget_delegate_->OnTouchEvent(event);
}

ui::EventResult DesktopNativeWidgetAura::OnGestureEvent(
    ui::GestureEvent* event) {
  return native_widget_delegate_->OnGestureEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, aura::ActivationDelegate implementation:

bool DesktopNativeWidgetAura::ShouldActivate(const ui::Event* event) {
  return can_activate_ && native_widget_delegate_->CanActivate();
}

void DesktopNativeWidgetAura::OnActivated() {
  if (GetWidget()->HasFocusManager())
    GetWidget()->GetFocusManager()->RestoreFocusedView();
  native_widget_delegate_->OnNativeWidgetActivationChanged(true);
  if (IsVisible() && GetWidget()->non_client_view())
    GetWidget()->non_client_view()->SchedulePaint();
}

void DesktopNativeWidgetAura::OnLostActive() {
  native_widget_delegate_->OnNativeWidgetActivationChanged(false);
  if (IsVisible() && GetWidget()->non_client_view())
    GetWidget()->non_client_view()->SchedulePaint();
}

}  // namespace views
