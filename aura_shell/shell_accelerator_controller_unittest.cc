// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura_shell/shell.h"
#include "ui/aura_shell/shell_accelerator_controller.h"
#include "ui/aura_shell/shell_window_ids.h"
#include "ui/aura_shell/test/aura_shell_test_base.h"
#include "ui/aura_shell/window_util.h"

#if defined(USE_X11)
#include <X11/Xlib.h>
#include "ui/base/x/x11_util.h"
#endif

namespace aura_shell {
namespace test {

namespace {
class TestTarget : public ui::AcceleratorTarget {
 public:
  TestTarget() : accelerator_pressed_count_(0) {};
  virtual ~TestTarget() {};

  int accelerator_pressed_count() const {
    return accelerator_pressed_count_;
  }

  void set_accelerator_pressed_count(int accelerator_pressed_count) {
    accelerator_pressed_count_ = accelerator_pressed_count;
  }

  // Overridden from ui::AcceleratorTarget:
  virtual bool AcceleratorPressed(const ui::Accelerator& accelerator) OVERRIDE;

 private:
  int accelerator_pressed_count_;
};

bool TestTarget::AcceleratorPressed(const ui::Accelerator& accelerator) {
  ++accelerator_pressed_count_;
  return true;
}

}  // namespace

class ShellAcceleratorControllerTest : public AuraShellTestBase {
 public:
  ShellAcceleratorControllerTest() {};
  virtual ~ShellAcceleratorControllerTest() {};

  static ShellAcceleratorController* GetController();
};

ShellAcceleratorController* ShellAcceleratorControllerTest::GetController() {
  return Shell::GetInstance()->accelerator_controller();
}

TEST_F(ShellAcceleratorControllerTest, Register) {
  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target;
  GetController()->Register(accelerator_a, &target);

  // The registered accelerator is processed.
  EXPECT_TRUE(GetController()->Process(accelerator_a));
  EXPECT_EQ(1, target.accelerator_pressed_count());
}

TEST_F(ShellAcceleratorControllerTest, RegisterMultipleTarget) {
  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target1;
  GetController()->Register(accelerator_a, &target1);
  TestTarget target2;
  GetController()->Register(accelerator_a, &target2);

  // If multiple targets are registered with the same accelerator, the target
  // registered later processes the accelerator.
  EXPECT_TRUE(GetController()->Process(accelerator_a));
  EXPECT_EQ(0, target1.accelerator_pressed_count());
  EXPECT_EQ(1, target2.accelerator_pressed_count());
}

TEST_F(ShellAcceleratorControllerTest, Unregister) {
  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target;
  GetController()->Register(accelerator_a, &target);
  const ui::Accelerator accelerator_b(ui::VKEY_B, false, false, false);
  GetController()->Register(accelerator_b, &target);

  // Unregistering a different accelerator does not affect the other
  // accelerator.
  GetController()->Unregister(accelerator_b, &target);
  EXPECT_TRUE(GetController()->Process(accelerator_a));
  EXPECT_EQ(1, target.accelerator_pressed_count());

  // The unregistered accelerator is no longer processed.
  target.set_accelerator_pressed_count(0);
  GetController()->Unregister(accelerator_a, &target);
  EXPECT_FALSE(GetController()->Process(accelerator_a));
  EXPECT_EQ(0, target.accelerator_pressed_count());
}

TEST_F(ShellAcceleratorControllerTest, UnregisterAll) {
  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target1;
  GetController()->Register(accelerator_a, &target1);
  const ui::Accelerator accelerator_b(ui::VKEY_B, false, false, false);
  GetController()->Register(accelerator_b, &target1);
  const ui::Accelerator accelerator_c(ui::VKEY_C, false, false, false);
  TestTarget target2;
  GetController()->Register(accelerator_c, &target2);
  GetController()->UnregisterAll(&target1);

  // All the accelerators registered for |target1| are no longer processed.
  EXPECT_FALSE(GetController()->Process(accelerator_a));
  EXPECT_FALSE(GetController()->Process(accelerator_b));
  EXPECT_EQ(0, target1.accelerator_pressed_count());

  // UnregisterAll with a different target does not affect the other target.
  EXPECT_TRUE(GetController()->Process(accelerator_c));
  EXPECT_EQ(1, target2.accelerator_pressed_count());
}

TEST_F(ShellAcceleratorControllerTest, Process) {
  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target1;
  GetController()->Register(accelerator_a, &target1);

  // The registered accelerator is processed.
  EXPECT_TRUE(GetController()->Process(accelerator_a));
  EXPECT_EQ(1, target1.accelerator_pressed_count());

  // The non-registered accelerator is not processed.
  const ui::Accelerator accelerator_b(ui::VKEY_B, false, false, false);
  EXPECT_FALSE(GetController()->Process(accelerator_b));
}

#if defined(OS_WIN) || defined(USE_X11)
TEST_F(ShellAcceleratorControllerTest, ProcessOnce) {
  // A focused window must exist for accelerators to be processed.
  aura::Window* default_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_DefaultContainer);
  aura::Window* window = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      default_container);
  ActivateWindow(window);

  const ui::Accelerator accelerator_a(ui::VKEY_A, false, false, false);
  TestTarget target;
  GetController()->Register(accelerator_a, &target);

  // The accelerator is processed only once.
#if defined(OS_WIN)
  MSG msg1 = { NULL, WM_KEYDOWN, ui::VKEY_A, 0 };
  aura::KeyEvent key_event1(msg1, false);
  EXPECT_TRUE(aura::RootWindow::GetInstance()->DispatchKeyEvent(&key_event1));

  MSG msg2 = { NULL, WM_CHAR, L'A', 0 };
  aura::KeyEvent key_event2(msg2, true);
  EXPECT_FALSE(aura::RootWindow::GetInstance()->DispatchKeyEvent(&key_event2));

  MSG msg3 = { NULL, WM_KEYUP, ui::VKEY_A, 0 };
  aura::KeyEvent key_event3(msg3, false);
  EXPECT_FALSE(aura::RootWindow::GetInstance()->DispatchKeyEvent(&key_event3));
#elif defined(USE_X11)
  XEvent key_event;
  ui::InitXKeyEventForTesting(ui::ET_KEY_PRESSED,
                              ui::VKEY_A,
                              0,
                              &key_event);
  EXPECT_TRUE(aura::RootWindow::GetInstance()->GetDispatcher()->Dispatch(
      &key_event));
#endif
  EXPECT_EQ(1, target.accelerator_pressed_count());
}
#endif

TEST_F(ShellAcceleratorControllerTest, GlobalAccelerators) {
  // A focused window must exist for accelerators to be processed.
  aura::Window* default_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_DefaultContainer);
  aura::Window* window = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      default_container);
  ActivateWindow(window);

  // CycleBackward
  EXPECT_TRUE(GetController()->Process(
      ui::Accelerator(ui::VKEY_TAB, true, false, true)));
  // CycleForwrard
  EXPECT_TRUE(GetController()->Process(
      ui::Accelerator(ui::VKEY_F5, false, false, false)));
  EXPECT_TRUE(GetController()->Process(
      ui::Accelerator(ui::VKEY_TAB, false, false, true)));
  // TakeScreenshot
  // EXPECT_TRUE(GetController()->Process(
  //     ui::Accelerator(ui::VKEY_F5, false, true, false)));
  // EXPECT_TRUE(GetController()->Process(
  //     ui::Accelerator(ui::VKEY_PRINT, false, false, false)));
#if !defined(NDEBUG)
  // RotateScreen
  EXPECT_TRUE(GetController()->Process(
      ui::Accelerator(ui::VKEY_HOME, false, true, false)));
#if !defined(OS_LINUX)
  // ToggleDesktopFullScreen (not implemented yet on Linux)
  EXPECT_TRUE(GetController()->Process(
      ui::Accelerator(ui::VKEY_F11, false, true, false)));
#endif
#endif
}

TEST_F(ShellAcceleratorControllerTest, HandleCycleWindow) {
  aura::Window* default_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_DefaultContainer);
  aura::Window* window0 = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      default_container);
  aura::Window* window1 = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      default_container);
  aura::Window* window2 = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      default_container);
  ActivateWindow(window0);
  EXPECT_TRUE(IsActiveWindow(window0));

  ui::Accelerator cycle_forward(ui::VKEY_TAB, false, false, true);
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_TRUE(IsActiveWindow(window1));
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_TRUE(IsActiveWindow(window2));
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_TRUE(IsActiveWindow(window0));

  ui::Accelerator cycle_backward(ui::VKEY_TAB, true, false, true);
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_TRUE(IsActiveWindow(window2));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_TRUE(IsActiveWindow(window1));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_TRUE(IsActiveWindow(window0));

  aura::Window* modal_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_AlwaysOnTopContainer);
  aura::Window* modal_window = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      modal_container);

  // When the modal window is active, cycling window does not take effect.
  ActivateWindow(modal_window);
  EXPECT_TRUE(IsActiveWindow(modal_window));
  EXPECT_FALSE(GetController()->Process(cycle_forward));
  EXPECT_TRUE(IsActiveWindow(modal_window));
  EXPECT_FALSE(IsActiveWindow(window0));
  EXPECT_FALSE(IsActiveWindow(window1));
  EXPECT_FALSE(IsActiveWindow(window2));
  EXPECT_FALSE(GetController()->Process(cycle_backward));
  EXPECT_TRUE(IsActiveWindow(modal_window));
  EXPECT_FALSE(IsActiveWindow(window0));
  EXPECT_FALSE(IsActiveWindow(window1));
  EXPECT_FALSE(IsActiveWindow(window2));

  // The modal window is not activated by cycling window.
  ActivateWindow(window0);
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_FALSE(IsActiveWindow(modal_window));
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_FALSE(IsActiveWindow(modal_window));
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_FALSE(IsActiveWindow(modal_window));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_FALSE(IsActiveWindow(modal_window));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_FALSE(IsActiveWindow(modal_window));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
  EXPECT_FALSE(IsActiveWindow(modal_window));

  // When a screen lock window is visible, cycling window does not take effect.
  aura::Window* lock_screen_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_LockScreenContainer);
  aura::Window* lock_screen_window = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      lock_screen_container);

  lock_screen_window->Show();
  EXPECT_FALSE(GetController()->Process(cycle_forward));
  EXPECT_FALSE(GetController()->Process(cycle_backward));

  // When a screen lock window is visible, cycling window does not take effect.
  // But otherwise, cycling window does take effect.
  aura::Window* lock_modal_container =
      aura_shell::Shell::GetInstance()->GetContainer(
          internal::kShellWindowId_LockModalContainer);
  aura::Window* lock_modal_window = aura::test::CreateTestWindowWithDelegate(
      new aura::test::TestWindowDelegate,
      -1,
      gfx::Rect(),
      lock_modal_container);

  lock_modal_window->Show();
  EXPECT_FALSE(GetController()->Process(cycle_forward));
  EXPECT_FALSE(GetController()->Process(cycle_backward));
  lock_screen_window->Hide();
  EXPECT_TRUE(GetController()->Process(cycle_forward));
  EXPECT_TRUE(GetController()->Process(cycle_backward));
}

}  // namespace test
}  // namespace aura_shell
