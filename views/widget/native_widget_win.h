// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_NATIVE_WIDGET_WIN_H_
#define UI_VIEWS_WIDGET_NATIVE_WIDGET_WIN_H_
#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlmisc.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/win/scoped_comptr.h"
#include "base/win/win_util.h"
#include "ui/base/win/window_impl.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/widget/native_widget_private.h"

namespace ui {
class Compositor;
class ViewProp;
}

namespace gfx {
class Canvas;
class Font;
class Rect;
}

namespace views {

class DropTargetWin;
class RootView;
class TooltipManagerWin;

// These two messages aren't defined in winuser.h, but they are sent to windows
// with captions. They appear to paint the window caption and frame.
// Unfortunately if you override the standard non-client rendering as we do
// with CustomFrameWindow, sometimes Windows (not deterministically
// reproducibly but definitely frequently) will send these messages to the
// window and paint the standard caption/title over the top of the custom one.
// So we need to handle these messages in CustomFrameWindow to prevent this
// from happening.
const int WM_NCUAHDRAWCAPTION = 0xAE;
const int WM_NCUAHDRAWFRAME = 0xAF;

///////////////////////////////////////////////////////////////////////////////
//
// NativeWidgetWin
//  A Widget for a views hierarchy used to represent anything that can be
//  contained within an HWND, e.g. a control, a window, etc. Specializations
//  suitable for specific tasks, e.g. top level window, are derived from this.
//
//  This Widget contains a RootView which owns the hierarchy of views within it.
//  As long as views are part of this tree, they will be deleted automatically
//  when the RootView is destroyed. If you remove a view from the tree, you are
//  then responsible for cleaning up after it.
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT NativeWidgetWin : public ui::WindowImpl,
                                     public MessageLoopForUI::Observer,
                                     public internal::NativeWidgetPrivate {
 public:
  explicit NativeWidgetWin(internal::NativeWidgetDelegate* delegate);
  virtual ~NativeWidgetWin();

  // Returns true if we are on Windows Vista or greater and composition is
  // enabled.
  static bool IsAeroGlassEnabled();

  // Returns the system set window title font.
  static gfx::Font GetWindowTitleFont();

  // Show the window with the specified show command.
  void Show(int show_state);

  // Disable Layered Window updates by setting to false.
  void set_can_update_layered_window(bool can_update_layered_window) {
    can_update_layered_window_ = can_update_layered_window;
  }

  // Obtain the view event with the given MSAA child id.  Used in
  // NativeViewAccessibilityWin::get_accChild to support requests for
  // children of windowless controls.  May return NULL
  // (see ViewHierarchyChanged).
  View* GetAccessibilityViewEventAt(int id);

  // Add a view that has recently fired an accessibility event.  Returns a MSAA
  // child id which is generated by: -(index of view in vector + 1) which
  // guarantees a negative child id.  This distinguishes the view from
  // positive MSAA child id's which are direct leaf children of views that have
  // associated hWnd's (e.g. NativeWidgetWin).
  int AddAccessibilityViewEvent(View* view);

  // Clear a view that has recently been removed on a hierarchy change.
  void ClearAccessibilityViewEvent(View* view);

  // Hides the window if it hasn't already been force-hidden. The force hidden
  // count is tracked, so calling multiple times is allowed, you just have to
  // be sure to call PopForceHidden the same number of times.
  void PushForceHidden();

  // Decrements the force hidden count, showing the window if we have reached
  // the top of the stack. See PushForceHidden.
  void PopForceHidden();

  BOOL IsWindow() const {
    return ::IsWindow(GetNativeView());
  }

  BOOL ShowWindow(int command) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::ShowWindow(GetNativeView(), command);
  }

  HWND GetParent() const {
    return ::GetParent(GetNativeView());
  }

  LONG GetWindowLong(int index) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::GetWindowLong(GetNativeView(), index);
  }

  BOOL GetWindowRect(RECT* rect) const {
    return ::GetWindowRect(GetNativeView(), rect);
  }

  LONG SetWindowLong(int index, LONG new_long) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowLong(GetNativeView(), index, new_long);
  }

  BOOL SetWindowPos(HWND hwnd_after, int x, int y, int cx, int cy, UINT flags) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowPos(GetNativeView(), hwnd_after, x, y, cx, cy, flags);
  }

  BOOL IsZoomed() const {
    DCHECK(::IsWindow(GetNativeView()));
    return ::IsZoomed(GetNativeView());
  }

  BOOL MoveWindow(int x, int y, int width, int height) {
    return MoveWindow(x, y, width, height, TRUE);
  }

  BOOL MoveWindow(int x, int y, int width, int height, BOOL repaint) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::MoveWindow(GetNativeView(), x, y, width, height, repaint);
  }

  int SetWindowRgn(HRGN region, BOOL redraw) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowRgn(GetNativeView(), region, redraw);
  }

  BOOL GetClientRect(RECT* rect) const {
    DCHECK(::IsWindow(GetNativeView()));
    return ::GetClientRect(GetNativeView(), rect);
  }

  // Overridden from internal::NativeWidgetPrivate:
  virtual void InitNativeWidget(const Widget::InitParams& params) OVERRIDE;
  virtual NonClientFrameView* CreateNonClientFrameView() OVERRIDE;
  virtual void UpdateFrameAfterFrameChange() OVERRIDE;
  virtual bool ShouldUseNativeFrame() const OVERRIDE;
  virtual void FrameTypeChanged() OVERRIDE;
  virtual Widget* GetWidget() OVERRIDE;
  virtual const Widget* GetWidget() const OVERRIDE;
  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual gfx::NativeWindow GetNativeWindow() const OVERRIDE;
  virtual Widget* GetTopLevelWidget() OVERRIDE;
  virtual const ui::Compositor* GetCompositor() const OVERRIDE;
  virtual ui::Compositor* GetCompositor() OVERRIDE;
  virtual void CalculateOffsetToAncestorWithLayer(
      gfx::Point* offset,
      ui::Layer** layer_parent) OVERRIDE;
  virtual void ViewRemoved(View* view) OVERRIDE;
  virtual void SetNativeWindowProperty(const char* name, void* value) OVERRIDE;
  virtual void* GetNativeWindowProperty(const char* name) const OVERRIDE;
  virtual TooltipManager* GetTooltipManager() const OVERRIDE;
  virtual bool IsScreenReaderActive() const OVERRIDE;
  virtual void SendNativeAccessibilityEvent(
      View* view,
      ui::AccessibilityTypes::Event event_type) OVERRIDE;
  // NativeWidgetWin ignores touch captures.
  virtual void SetCapture(unsigned int flags) OVERRIDE;
  virtual void ReleaseCapture() OVERRIDE;
  virtual bool HasCapture(unsigned int flags) const OVERRIDE;
  virtual InputMethod* CreateInputMethod() OVERRIDE;
  virtual void CenterWindow(const gfx::Size& size) OVERRIDE;
  virtual void GetWindowPlacement(
     gfx::Rect* bounds,
     ui::WindowShowState* show_state) const OVERRIDE;
  virtual void SetWindowTitle(const string16& title) OVERRIDE;
  virtual void SetWindowIcons(const SkBitmap& window_icon,
                              const SkBitmap& app_icon) OVERRIDE;
  virtual void SetAccessibleName(const string16& name) OVERRIDE;
  virtual void SetAccessibleRole(ui::AccessibilityTypes::Role role) OVERRIDE;
  virtual void SetAccessibleState(ui::AccessibilityTypes::State state) OVERRIDE;
  virtual void InitModalType(ui::ModalType modal_type) OVERRIDE;
  virtual gfx::Rect GetWindowScreenBounds() const OVERRIDE;
  virtual gfx::Rect GetClientAreaScreenBounds() const OVERRIDE;
  virtual gfx::Rect GetRestoredBounds() const OVERRIDE;
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual void StackAbove(gfx::NativeView native_view) OVERRIDE;
  virtual void StackAtTop() OVERRIDE;
  virtual void StackBelow(gfx::NativeView native_view) OVERRIDE;
  virtual void SetShape(gfx::NativeRegion shape) OVERRIDE;
  virtual void Close() OVERRIDE;
  virtual void CloseNow() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual void ShowMaximizedWithBounds(
      const gfx::Rect& restored_bounds) OVERRIDE;
  virtual void ShowWithWindowState(ui::WindowShowState show_state) OVERRIDE;
  virtual bool IsVisible() const OVERRIDE;
  virtual void Activate() OVERRIDE;
  virtual void Deactivate() OVERRIDE;
  virtual bool IsActive() const OVERRIDE;
  virtual void SetAlwaysOnTop(bool always_on_top) OVERRIDE;
  virtual void Maximize() OVERRIDE;
  virtual void Minimize() OVERRIDE;
  virtual bool IsMaximized() const OVERRIDE;
  virtual bool IsMinimized() const OVERRIDE;
  virtual void Restore() OVERRIDE;
  virtual void SetFullscreen(bool fullscreen) OVERRIDE;
  virtual bool IsFullscreen() const OVERRIDE;
  virtual void SetOpacity(unsigned char opacity) OVERRIDE;
  virtual void SetUseDragFrame(bool use_drag_frame) OVERRIDE;
  virtual void FlashFrame(bool flash) OVERRIDE;
  virtual bool IsAccessibleWidget() const OVERRIDE;
  virtual void RunShellDrag(View* view,
                            const ui::OSExchangeData& data,
                            const gfx::Point& location,
                            int operation) OVERRIDE;
  virtual void SchedulePaintInRect(const gfx::Rect& rect) OVERRIDE;
  virtual void SetCursor(gfx::NativeCursor cursor) OVERRIDE;
  virtual void ClearNativeFocus() OVERRIDE;
  virtual void FocusNativeView(gfx::NativeView native_view) OVERRIDE;
  virtual gfx::Rect GetWorkAreaBoundsInScreen() const OVERRIDE;
  virtual void SetInactiveRenderingDisabled(bool value) OVERRIDE;
  virtual Widget::MoveLoopResult RunMoveLoop() OVERRIDE;
  virtual void EndMoveLoop() OVERRIDE;
  virtual void SetVisibilityChangedAnimationsEnabled(bool value) OVERRIDE;

 protected:
  // Information saved before going into fullscreen mode, used to restore the
  // window afterwards.
  struct SavedWindowInfo {
    bool maximized;
    LONG style;
    LONG ex_style;
    RECT window_rect;
  };

  // Overridden from MessageLoop::Observer:
  virtual base::EventStatus WillProcessEvent(
      const base::NativeEvent& event) OVERRIDE;
  virtual void DidProcessEvent(const base::NativeEvent& event) OVERRIDE;

  // Overridden from WindowImpl:
  virtual HICON GetDefaultWindowIcon() const OVERRIDE;
  virtual LRESULT OnWndProc(UINT message,
                            WPARAM w_param,
                            LPARAM l_param) OVERRIDE;

  // Message Handlers ----------------------------------------------------------

  BEGIN_MSG_MAP_EX(NativeWidgetWin)
    // Range handlers must go first!
    MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseRange)
    MESSAGE_RANGE_HANDLER_EX(WM_NCMOUSEMOVE, WM_NCXBUTTONDBLCLK, OnMouseRange)

    // Reflected message handler
    MESSAGE_HANDLER_EX(base::win::kReflectedMessage, OnReflectedMessage)

    // CustomFrameWindow hacks
    MESSAGE_HANDLER_EX(WM_NCUAHDRAWCAPTION, OnNCUAHDrawCaption)
    MESSAGE_HANDLER_EX(WM_NCUAHDRAWFRAME, OnNCUAHDrawFrame)

    // Vista and newer
    MESSAGE_HANDLER_EX(WM_DWMCOMPOSITIONCHANGED, OnDwmCompositionChanged)

    // Non-atlcrack.h handlers
    MESSAGE_HANDLER_EX(WM_GETOBJECT, OnGetObject)

    // Mouse events.
    MESSAGE_HANDLER_EX(WM_MOUSEACTIVATE, OnMouseActivate)
    MESSAGE_HANDLER_EX(WM_MOUSELEAVE, OnMouseRange)
    MESSAGE_HANDLER_EX(WM_NCMOUSELEAVE, OnMouseRange)
    MESSAGE_HANDLER_EX(WM_SETCURSOR, OnSetCursor);

    // Key events.
    MESSAGE_HANDLER_EX(WM_KEYDOWN, OnKeyEvent)
    MESSAGE_HANDLER_EX(WM_KEYUP, OnKeyEvent)
    MESSAGE_HANDLER_EX(WM_SYSKEYDOWN, OnKeyEvent)
    MESSAGE_HANDLER_EX(WM_SYSKEYUP, OnKeyEvent)

    // IME Events.
    MESSAGE_HANDLER_EX(WM_IME_SETCONTEXT, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_IME_STARTCOMPOSITION, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_IME_COMPOSITION, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_IME_ENDCOMPOSITION, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_IME_REQUEST, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_CHAR, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_SYSCHAR, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_DEADCHAR, OnImeMessages)
    MESSAGE_HANDLER_EX(WM_SYSDEADCHAR, OnImeMessages)

    // This list is in _ALPHABETICAL_ order! OR I WILL HURT YOU.
    MSG_WM_ACTIVATE(OnActivate)
    MSG_WM_ACTIVATEAPP(OnActivateApp)
    MSG_WM_APPCOMMAND(OnAppCommand)
    MSG_WM_CANCELMODE(OnCancelMode)
    MSG_WM_CAPTURECHANGED(OnCaptureChanged)
    MSG_WM_CLOSE(OnClose)
    MSG_WM_COMMAND(OnCommand)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_DISPLAYCHANGE(OnDisplayChange)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    MSG_WM_ENDSESSION(OnEndSession)
    MSG_WM_ENTERSIZEMOVE(OnEnterSizeMove)
    MSG_WM_EXITMENULOOP(OnExitMenuLoop)
    MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
    MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
    MSG_WM_HSCROLL(OnHScroll)
    MSG_WM_INITMENU(OnInitMenu)
    MSG_WM_INITMENUPOPUP(OnInitMenuPopup)
    MSG_WM_INPUTLANGCHANGE(OnInputLangChange)
    MSG_WM_KILLFOCUS(OnKillFocus)
    MSG_WM_MOVE(OnMove)
    MSG_WM_MOVING(OnMoving)
    MSG_WM_NCACTIVATE(OnNCActivate)
    MSG_WM_NCCALCSIZE(OnNCCalcSize)
    MSG_WM_NCHITTEST(OnNCHitTest)
    MSG_WM_NCPAINT(OnNCPaint)
    MSG_WM_NOTIFY(OnNotify)
    MSG_WM_PAINT(OnPaint)
    MSG_WM_POWERBROADCAST(OnPowerBroadcast)
    MSG_WM_SETFOCUS(OnSetFocus)
    MSG_WM_SETTEXT(OnSetText)
    MSG_WM_SETTINGCHANGE(OnSettingChange)
    MSG_WM_SIZE(OnSize)
    MSG_WM_SYSCOMMAND(OnSysCommand)
    MSG_WM_THEMECHANGED(OnThemeChanged)
    MSG_WM_VSCROLL(OnVScroll)
    MSG_WM_WINDOWPOSCHANGING(OnWindowPosChanging)
    MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
  END_MSG_MAP()

  // These are all virtual so that specialized Widgets can modify or augment
  // processing.
  // This list is in _ALPHABETICAL_ order!
  // Note: in the base class these functions must do nothing but convert point
  //       coordinates to client coordinates (if necessary) and forward the
  //       handling to the appropriate Process* function. This is so that
  //       subclasses can easily override these methods to do different things
  //       and have a convenient function to call to get the default behavior.
  virtual void OnActivate(UINT action, BOOL minimized, HWND window);
  virtual void OnActivateApp(BOOL active, DWORD thread_id);
  virtual LRESULT OnAppCommand(HWND window, short app_command, WORD device,
                               int keystate);
  virtual void OnCancelMode();
  virtual void OnCaptureChanged(HWND hwnd);
  virtual void OnClose();
  virtual void OnCommand(UINT notification_code, int command_id, HWND window);
  virtual LRESULT OnCreate(CREATESTRUCT* create_struct);
  // WARNING: If you override this be sure and invoke super, otherwise we'll
  // leak a few things.
  virtual void OnDestroy();
  virtual void OnDisplayChange(UINT bits_per_pixel, CSize screen_size);
  virtual LRESULT OnDwmCompositionChanged(UINT msg,
                                          WPARAM w_param,
                                          LPARAM l_param);
  virtual void OnEndSession(BOOL ending, UINT logoff);
  virtual void OnEnterSizeMove();
  virtual LRESULT OnEraseBkgnd(HDC dc);
  virtual void OnExitMenuLoop(BOOL is_track_popup_menu);
  virtual void OnExitSizeMove();
  virtual LRESULT OnGetObject(UINT uMsg, WPARAM w_param, LPARAM l_param);
  virtual void OnGetMinMaxInfo(MINMAXINFO* minmax_info);
  virtual void OnHScroll(int scroll_type, short position, HWND scrollbar);
  virtual LRESULT OnImeMessages(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void OnInitMenu(HMENU menu);
  virtual void OnInitMenuPopup(HMENU menu, UINT position, BOOL is_system_menu);
  virtual void OnInputLangChange(DWORD character_set, HKL input_language_id);
  virtual LRESULT OnKeyEvent(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void OnKillFocus(HWND focused_window);
  virtual LRESULT OnMouseActivate(UINT message, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnMouseRange(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void OnMove(const CPoint& point);
  virtual void OnMoving(UINT param, LPRECT new_bounds);
  virtual LRESULT OnNCActivate(BOOL active);
  virtual LRESULT OnNCCalcSize(BOOL w_param, LPARAM l_param);
  virtual LRESULT OnNCHitTest(const CPoint& pt);
  virtual void OnNCPaint(HRGN rgn);
  virtual LRESULT OnNCUAHDrawCaption(UINT msg,
                                     WPARAM w_param,
                                     LPARAM l_param);
  virtual LRESULT OnNCUAHDrawFrame(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnNotify(int w_param, NMHDR* l_param);
  virtual void OnPaint(HDC dc);
  virtual LRESULT OnPowerBroadcast(DWORD power_event, DWORD data);
  virtual LRESULT OnReflectedMessage(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnSetCursor(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void OnSetFocus(HWND focused_window);
  virtual LRESULT OnSetText(const wchar_t* text);
  virtual void OnSettingChange(UINT flags, const wchar_t* section);
  virtual void OnSize(UINT param, const CSize& size);
  virtual void OnSysCommand(UINT notification_code, CPoint click);
  virtual void OnThemeChanged();
  virtual void OnVScroll(int scroll_type, short position, HWND scrollbar);
  virtual void OnWindowPosChanging(WINDOWPOS* window_pos);
  virtual void OnWindowPosChanged(WINDOWPOS* window_pos);

  // Deletes this window as it is destroyed, override to provide different
  // behavior.
  virtual void OnFinalMessage(HWND window);

  // Retrieve the show state of the window. This is one of the SW_SHOW* flags
  // passed into Windows' ShowWindow method. For normal windows this defaults
  // to SW_SHOWNORMAL, however windows (e.g. the main window) can override this
  // method to provide different values (e.g. retrieve the user's specified
  // show state from the shortcut starutp info).
  virtual int GetShowState() const;

  // Returns the insets of the client area relative to the non-client area of
  // the window. Override this function instead of OnNCCalcSize, which is
  // crazily complicated.
  virtual gfx::Insets GetClientAreaInsets() const;

  // Start tracking all mouse events so that this window gets sent mouse leave
  // messages too.
  void TrackMouseEvents(DWORD mouse_tracking_flags);

  // Called when a MSAA screen reader client is detected.
  virtual void OnScreenReaderDetected();

  // Executes the specified SC_command.
  void ExecuteSystemMenuCommand(int command);

  // The TooltipManager. This is NULL if there is a problem creating the
  // underlying tooltip window.
  // WARNING: RootView's destructor calls into the TooltipManager. As such, this
  // must be destroyed AFTER root_view_.
  scoped_ptr<TooltipManagerWin> tooltip_manager_;

  scoped_refptr<DropTargetWin> drop_target_;

  const gfx::Rect& invalid_rect() const { return invalid_rect_; }

  // Saved window information from before entering fullscreen mode.
  // TODO(beng): move to private once GetRestoredBounds() moves onto Widget.
  SavedWindowInfo saved_window_info_;

 private:
  typedef ScopedVector<ui::ViewProp> ViewProps;

  // Called after the WM_ACTIVATE message has been processed by the default
  // windows procedure.
  static void PostProcessActivateMessage(NativeWidgetWin* widget,
                                         int activation_state);

  void SetInitParams(const Widget::InitParams& params);

  // Synchronously paints the invalid contents of the Widget.
  void RedrawInvalidRect();

  // Synchronously updates the invalid contents of the Widget. Valid for
  // layered windows only.
  void RedrawLayeredWindowContents();

  // Lock or unlock the window from being able to redraw itself in response to
  // updates to its invalid region.
  class ScopedRedrawLock;
  void LockUpdates(bool force);
  void UnlockUpdates(bool force);

  // Determines whether the delegate expects the client size or the window size.
  bool WidgetSizeIsClientSize() const;

  // Responds to the client area changing size, either at window creation time
  // or subsequently.
  void ClientAreaSizeChanged();

  // Resets the window region for the current widget bounds if necessary.
  // If |force| is true, the window region is reset to NULL even for native
  // frame windows.
  void ResetWindowRegion(bool force);

  // When removing the standard frame, tells the DWM how much glass we want on
  // the edges. Currently hardcoded to 10px on all sides.
  void UpdateDWMFrame();

  // Calls DefWindowProc, safely wrapping the call in a ScopedRedrawLock to
  // prevent frame flicker. DefWindowProc handling can otherwise render the
  // classic-look window title bar directly.
  LRESULT DefWindowProcWithRedrawLock(UINT message,
                                      WPARAM w_param,
                                      LPARAM l_param);

  // Stops ignoring SetWindowPos() requests (see below).
  void StopIgnoringPosChanges() { ignore_window_pos_changes_ = false; }

  void RestoreEnabledIfNecessary();

  void SetInitialFocus();

  // Notifies any owned windows that we're closing.
  void NotifyOwnedWindowsParentClosing();

  // Overridden from internal::InputMethodDelegate
  virtual void DispatchKeyEventPostIME(const KeyEvent& key) OVERRIDE;

  // A delegate implementation that handles events received here.
  // See class documentation for Widget in widget.h for a note about ownership.
  internal::NativeWidgetDelegate* delegate_;

  // The following factory is used for calls to close the NativeWidgetWin
  // instance.
  base::WeakPtrFactory<NativeWidgetWin> close_widget_factory_;

  // The flags currently being used with TrackMouseEvent to track mouse
  // messages. 0 if there is no active tracking. The value of this member is
  // used when tracking is canceled.
  DWORD active_mouse_tracking_flags_;

  // Should we keep an off-screen buffer? This is false by default, set to true
  // when WS_EX_LAYERED is specified before the native window is created.
  //
  // NOTE: this is intended to be used with a layered window (a window with an
  // extended window style of WS_EX_LAYERED). If you are using a layered window
  // and NOT changing the layered alpha or anything else, then leave this value
  // alone. OTOH if you are invoking SetLayeredWindowAttributes then you'll
  // most likely want to set this to false, or after changing the alpha toggle
  // the extended style bit to false than back to true. See MSDN for more
  // details.
  bool use_layered_buffer_;

  // The default alpha to be applied to the layered window.
  BYTE layered_alpha_;

  // A canvas that contains the window contents in the case of a layered
  // window.
  scoped_ptr<gfx::Canvas> layered_window_contents_;

  // We must track the invalid rect ourselves, for two reasons:
  // For layered windows, Windows will not do this properly with
  // InvalidateRect()/GetUpdateRect(). (In fact, it'll return misleading
  // information from GetUpdateRect()).
  // We also need to keep track of the invalid rectangle for the RootView should
  // we need to paint the non-client area. The data supplied to WM_NCPAINT seems
  // to be insufficient.
  gfx::Rect invalid_rect_;

  // A factory that allows us to schedule a redraw for layered windows.
  base::WeakPtrFactory<NativeWidgetWin> paint_layered_window_factory_;

  // See class documentation for Widget in widget.h for a note about ownership.
  Widget::InitParams::Ownership ownership_;

  // True if we are allowed to update the layered window from the DIB backing
  // store if necessary.
  bool can_update_layered_window_;

  // Whether the focus should be restored next time we get enabled.  Needed to
  // restore focus correctly when Windows modal dialogs are displayed.
  bool restore_focus_when_enabled_;

  // Instance of accessibility information and handling for MSAA root
  base::win::ScopedComPtr<IAccessible> accessibility_root_;

  // Value determines whether the Widget is customized for accessibility.
  static bool screen_reader_active_;

  // The maximum number of view events in our vector below.
  static const int kMaxAccessibilityViewEvents = 20;

  // A vector used to access views for which we have sent notifications to
  // accessibility clients.  It is used as a circular queue.
  std::vector<View*> accessibility_view_events_;

  // The current position of the view events vector.  When incrementing,
  // we always mod this value with the max view events above .
  int accessibility_view_events_index_;

  // The last cursor that was active before the current one was selected. Saved
  // so that we can restore it.
  gfx::NativeCursor previous_cursor_;

  ViewProps props_;

  // True if we're in fullscreen mode.
  bool fullscreen_;

  // If this is greater than zero, we should prevent attempts to make the window
  // visible when we handle WM_WINDOWPOSCHANGING. Some calls like
  // ShowWindow(SW_RESTORE) make the window visible in addition to restoring it,
  // when all we want to do is restore it.
  int force_hidden_count_;

  // The window styles before we modified them for the drag frame appearance.
  DWORD drag_frame_saved_window_style_;
  DWORD drag_frame_saved_window_ex_style_;

  // Represents the number of ScopedRedrawLocks active against this widget.
  // If this is greater than zero, the widget should be locked against updates.
  int lock_updates_count_;

  // When true, this flag makes us discard incoming SetWindowPos() requests that
  // only change our position/size.  (We still allow changes to Z-order,
  // activation, etc.)
  bool ignore_window_pos_changes_;

  // The following factory is used to ignore SetWindowPos() calls for short time
  // periods.
  base::WeakPtrFactory<NativeWidgetWin> ignore_pos_changes_factory_;

  // The last-seen monitor containing us, and its rect and work area.  These are
  // used to catch updates to the rect and work area and react accordingly.
  HMONITOR last_monitor_;
  gfx::Rect last_monitor_rect_, last_work_area_;

  // Set to true when the user presses the right mouse button on the caption
  // area. We need this so we can correctly show the context menu on mouse-up.
  bool is_right_mouse_pressed_on_caption_;

  // Whether all ancestors have been enabled. This is only used if is_modal_ is
  // true.
  bool restored_enabled_;

  // This flag can be initialized and checked after certain operations (such as
  // DefWindowProc) to avoid stack-controlled NativeWidgetWin operations (such
  // as unlocking the Window with a ScopedRedrawLock) after Widget destruction.
  bool* destroyed_;

  // True if the widget is going to have a non_client_view. We cache this value
  // rather than asking the Widget for the non_client_view so that we know at
  // Init time, before the Widget has created the NonClientView.
  bool has_non_client_view_;

  bool remove_standard_frame_;

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetWin);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_NATIVE_WIDGET_WIN_H_
