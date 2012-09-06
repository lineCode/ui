// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_native_widget_aura.h"

#include "ui/base/hit_test.h"
#include "ui/views/widget/desktop_root_window_host.h"

namespace views {

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, public:

DesktopNativeWidgetAura::DesktopNativeWidgetAura(
    internal::NativeWidgetDelegate* delegate)
    : desktop_root_window_host_(DesktopRootWindowHost::Create()),
      window_(NULL),
      ownership_(Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET) {
}

DesktopNativeWidgetAura::~DesktopNativeWidgetAura() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, internal::NativeWidgetPrivate implementation:

void DesktopNativeWidgetAura::InitNativeWidget(
    const Widget::InitParams& params) {
}

NonClientFrameView* DesktopNativeWidgetAura::CreateNonClientFrameView() {
  return NULL;
}

bool DesktopNativeWidgetAura::ShouldUseNativeFrame() const {
  return false;
}

void DesktopNativeWidgetAura::FrameTypeChanged() {
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
  return NULL;
}

ui::Compositor* DesktopNativeWidgetAura::GetCompositor() {
  return NULL;
}

void DesktopNativeWidgetAura::CalculateOffsetToAncestorWithLayer(
      gfx::Point* offset,
      ui::Layer** layer_parent) {
}

void DesktopNativeWidgetAura::ViewRemoved(View* view) {
}

void DesktopNativeWidgetAura::SetNativeWindowProperty(const char* name,
                                                      void* value) {
}

void* DesktopNativeWidgetAura::GetNativeWindowProperty(const char* name) const {
  return NULL;
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
}

void DesktopNativeWidgetAura::ReleaseCapture() {
}

bool DesktopNativeWidgetAura::HasCapture() const {
  return false;
}

InputMethod* DesktopNativeWidgetAura::CreateInputMethod() {
  return NULL;
}

internal::InputMethodDelegate*
    DesktopNativeWidgetAura::GetInputMethodDelegate() {
  return NULL;
}

void DesktopNativeWidgetAura::CenterWindow(const gfx::Size& size) {
}

void DesktopNativeWidgetAura::GetWindowPlacement(
      gfx::Rect* bounds,
      ui::WindowShowState* maximized) const {
}

void DesktopNativeWidgetAura::SetWindowTitle(const string16& title) {
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
  return gfx::Rect(100, 100);
}

gfx::Rect DesktopNativeWidgetAura::GetClientAreaBoundsInScreen() const {
  return gfx::Rect(100, 100);
}

gfx::Rect DesktopNativeWidgetAura::GetRestoredBounds() const {
  return gfx::Rect(100, 100);
}

void DesktopNativeWidgetAura::SetBounds(const gfx::Rect& bounds) {
}

void DesktopNativeWidgetAura::SetSize(const gfx::Size& size) {
}

void DesktopNativeWidgetAura::StackAbove(gfx::NativeView native_view) {
}

void DesktopNativeWidgetAura::StackAtTop() {
}

void DesktopNativeWidgetAura::StackBelow(gfx::NativeView native_view) {
}

void DesktopNativeWidgetAura::SetShape(gfx::NativeRegion shape) {
}

void DesktopNativeWidgetAura::Close() {
}

void DesktopNativeWidgetAura::CloseNow() {
}

void DesktopNativeWidgetAura::Show() {
}

void DesktopNativeWidgetAura::Hide() {
}

void DesktopNativeWidgetAura::ShowMaximizedWithBounds(
      const gfx::Rect& restored_bounds) {
}

void DesktopNativeWidgetAura::ShowWithWindowState(ui::WindowShowState state) {
}

bool DesktopNativeWidgetAura::IsVisible() const {
  return false;
}

void DesktopNativeWidgetAura::Activate() {
}

void DesktopNativeWidgetAura::Deactivate() {
}

bool DesktopNativeWidgetAura::IsActive() const {
  return false;
}

void DesktopNativeWidgetAura::SetAlwaysOnTop(bool always_on_top) {
}

void DesktopNativeWidgetAura::Maximize() {
}

void DesktopNativeWidgetAura::Minimize() {
}

bool DesktopNativeWidgetAura::IsMaximized() const {
  return false;
}

bool DesktopNativeWidgetAura::IsMinimized() const {
  return false;
}

void DesktopNativeWidgetAura::Restore() {
}

void DesktopNativeWidgetAura::SetFullscreen(bool fullscreen) {
}

bool DesktopNativeWidgetAura::IsFullscreen() const {
  return false;
}

void DesktopNativeWidgetAura::SetOpacity(unsigned char opacity) {
}

void DesktopNativeWidgetAura::SetUseDragFrame(bool use_drag_frame) {
}

void DesktopNativeWidgetAura::FlashFrame(bool flash_frame) {
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
}

void DesktopNativeWidgetAura::SetCursor(gfx::NativeCursor cursor) {
}

void DesktopNativeWidgetAura::ClearNativeFocus() {
}

void DesktopNativeWidgetAura::FocusNativeView(gfx::NativeView native_view) {
}

gfx::Rect DesktopNativeWidgetAura::GetWorkAreaBoundsInScreen() const {
  return gfx::Rect(100, 100);
}

void DesktopNativeWidgetAura::SetInactiveRenderingDisabled(bool value) {
}

Widget::MoveLoopResult DesktopNativeWidgetAura::RunMoveLoop(
      const gfx::Point& drag_offset) {
  return Widget::MOVE_LOOP_CANCELED;
}

void DesktopNativeWidgetAura::EndMoveLoop() {
}

void DesktopNativeWidgetAura::SetVisibilityChangedAnimationsEnabled(
    bool value) {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetAura, aura::WindowDelegate implementation:

gfx::Size DesktopNativeWidgetAura::GetMinimumSize() const {
  return gfx::Size(100, 100);
}

void DesktopNativeWidgetAura::OnBoundsChanged(const gfx::Rect& old_bounds,
                               const gfx::Rect& new_bounds) {
}

void DesktopNativeWidgetAura::OnFocus(aura::Window* old_focused_window) {
}

void DesktopNativeWidgetAura::OnBlur() {
}

bool DesktopNativeWidgetAura::OnKeyEvent(ui::KeyEvent* event) {
  return false;
}

gfx::NativeCursor DesktopNativeWidgetAura::GetCursor(const gfx::Point& point) {
  return gfx::kNullCursor;
}

int DesktopNativeWidgetAura::GetNonClientComponent(
    const gfx::Point& point) const {
  return HTCLIENT;
}

bool DesktopNativeWidgetAura::ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) {
  return true;
}

bool DesktopNativeWidgetAura::OnMouseEvent(ui::MouseEvent* event) {
  return false;
}

ui::TouchStatus DesktopNativeWidgetAura::OnTouchEvent(ui::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::EventResult DesktopNativeWidgetAura::OnGestureEvent(
    ui::GestureEvent* event) {
  return ui::ER_UNHANDLED;
}

bool DesktopNativeWidgetAura::CanFocus() {
  return true;
}

void DesktopNativeWidgetAura::OnCaptureLost() {
}

void DesktopNativeWidgetAura::OnPaint(gfx::Canvas* canvas) {
}

void DesktopNativeWidgetAura::OnDeviceScaleFactorChanged(
    float device_scale_factor) {
}

void DesktopNativeWidgetAura::OnWindowDestroying() {
}

void DesktopNativeWidgetAura::OnWindowDestroyed() {
}

void DesktopNativeWidgetAura::OnWindowTargetVisibilityChanged(bool visible) {
}

bool DesktopNativeWidgetAura::HasHitTestMask() const {
  return false;
}

void DesktopNativeWidgetAura::GetHitTestMask(gfx::Path* mask) const {
}

}  // namespace views
