// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/corewm/focus_controller.h"

#include <map>

#include "ui/aura/client/activation_client.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/event_generator.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/base/events/event_handler.h"
#include "ui/views/corewm/base_focus_rules.h"
#include "ui/views/corewm/focus_change_event.h"

namespace views {
namespace corewm {

class FocusEventsTestHandler : public ui::EventHandler,
                               public aura::WindowObserver {
 public:
  explicit FocusEventsTestHandler(aura::Window* window)
      : window_(window),
        result_(ui::ER_UNHANDLED) {
    window_->AddObserver(this);
    window_->AddPreTargetHandler(this);
  }
  virtual ~FocusEventsTestHandler() {
    RemoveObserver();
  }

  void set_result(ui::EventResult result) { result_ = result; }

  int GetCountForEventType(int event_type) {
    std::map<int, int>::const_iterator it = event_counts_.find(event_type);
    return it != event_counts_.end() ? it->second : 0;
  }

 private:
  // Overridden from ui::EventHandler:
  virtual ui::EventResult OnEvent(ui::Event* event) OVERRIDE {
    event_counts_[event->type()] += 1;
    return result_;
  }

  // Overridden from aura::WindowObserver:
  virtual void OnWindowDestroyed(aura::Window* window) OVERRIDE {
    DCHECK_EQ(window, window_);
    RemoveObserver();
  }

  void RemoveObserver() {
    if (window_) {
      window_->RemoveObserver(this);
      window_->RemovePreTargetHandler(this);
      window_ = NULL;
    }
  }

  aura::Window* window_;
  std::map<int, int> event_counts_;
  ui::EventResult result_;

  DISALLOW_COPY_AND_ASSIGN(FocusEventsTestHandler);
};

// BaseFocusRules subclass that allows basic overrides of focus/activation to
// be tested. This is intended more as a test that the override system works at
// all, rather than as an exhaustive set of use cases, those should be covered
// in tests for those FocusRules implementations.
class TestFocusRules : public BaseFocusRules {
 public:
  TestFocusRules() : focus_restriction_(NULL) {}

  // Restricts focus and activation to this window and its child hierarchy.
  void set_focus_restriction(aura::Window* focus_restriction) {
    focus_restriction_ = focus_restriction;
  }

  // Overridden from BaseFocusRules:
  virtual bool CanActivateWindow(aura::Window* window) OVERRIDE {
    // Restricting focus to a non-activatable child window means the activatable
    // parent outside the focus restriction is activatable.
    bool can_activate = CanFocusOrActivate(window) ||
       window->Contains(GetActivatableWindow(focus_restriction_));
    return can_activate ? BaseFocusRules::CanActivateWindow(window) : false;
  }
  virtual bool CanFocusWindow(aura::Window* window) OVERRIDE {
    return CanFocusOrActivate(window) ?
        BaseFocusRules::CanFocusWindow(window) : false;
  }
  virtual aura::Window* GetActivatableWindow(aura::Window* window) OVERRIDE {
    return BaseFocusRules::GetActivatableWindow(
        CanFocusOrActivate(window) ? window : focus_restriction_);
  }
  virtual aura::Window* GetFocusableWindow(aura::Window* window) OVERRIDE {
    return BaseFocusRules::GetFocusableWindow(
        CanFocusOrActivate(window) ? window : focus_restriction_);
  }
  virtual aura::Window* GetNextActivatableWindow(
      aura::Window* ignore) OVERRIDE {
    aura::Window* next_activatable =
        BaseFocusRules::GetNextActivatableWindow(ignore);
    return CanFocusOrActivate(next_activatable) ?
        next_activatable : GetActivatableWindow(focus_restriction_);
  }
  virtual aura::Window* GetNextFocusableWindow(aura::Window* ignore) OVERRIDE {
    aura::Window* next_focusable =
        BaseFocusRules::GetNextFocusableWindow(ignore);
    return CanFocusOrActivate(next_focusable) ?
        next_focusable : focus_restriction_;
  }

 private:
  bool CanFocusOrActivate(aura::Window* window) const {
    return !focus_restriction_ || focus_restriction_->Contains(window);
  }

  aura::Window* focus_restriction_;

  DISALLOW_COPY_AND_ASSIGN(TestFocusRules);
};

// Common infrastructure shared by all FocusController test types.
class FocusControllerTestBase : public aura::test::AuraTestBase {
 protected:
  FocusControllerTestBase() {}

  // Overridden from aura::test::AuraTestBase:
  virtual void SetUp() OVERRIDE {
    // FocusController registers itself as an Env observer so it can catch all
    // window initializations, including the root_window()'s, so we create it
    // before allowing the base setup.
    test_focus_rules_ = new TestFocusRules;
    focus_controller_.reset(new FocusController(test_focus_rules_));
    aura::test::AuraTestBase::SetUp();
    root_window()->AddPreTargetHandler(focus_controller());
    aura::client::SetActivationClient(root_window(), focus_controller());

    // Hierarchy used by all tests:
    // root_window
    //       +-- w1
    //       |    +-- w11
    //       |    +-- w12
    //       +-- w2
    //       |    +-- w21
    //       |         +-- w211
    //       +-- w3
    aura::Window* w1 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 1,
        gfx::Rect(0, 0, 50, 50), NULL);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 11,
        gfx::Rect(5, 5, 10, 10), w1);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 12,
        gfx::Rect(15, 15, 10, 10), w1);
    aura::Window* w2 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 2,
        gfx::Rect(75, 75, 50, 50), NULL);
    aura::Window* w21 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 21,
        gfx::Rect(5, 5, 10, 10), w2);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 211,
        gfx::Rect(1, 1, 5, 5), w21);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 3,
        gfx::Rect(125, 125, 50, 50), NULL);
  }
  virtual void TearDown() OVERRIDE {
    root_window()->RemovePreTargetHandler(focus_controller());
    aura::test::AuraTestBase::TearDown();
    test_focus_rules_ = NULL;  // Owned by FocusController.
    focus_controller_.reset();
  }

  FocusController* focus_controller() { return focus_controller_.get(); }
  aura::Window* focused_window() { return focus_controller_->focused_window(); }
  int focused_window_id() {
      return focused_window() ? focused_window()->id() : -1;
  }

  void ActivateWindow(aura::Window* window) {
    aura::client::GetActivationClient(root_window())->ActivateWindow(window);
  }
  void DeactivateWindow(aura::Window* window) {
    aura::client::GetActivationClient(root_window())->DeactivateWindow(window);
  }
  aura::Window* GetActiveWindow() {
    return aura::client::GetActivationClient(root_window())->GetActiveWindow();
  }
  int GetActiveWindowId() {
    aura::Window* active_window = GetActiveWindow();
    return active_window ? active_window->id() : -1;
  }

  void ExpectActivationEvents(FocusEventsTestHandler* handler,
                              int expected_changing_event_count,
                              int expected_changed_event_count) {
    EXPECT_EQ(expected_changing_event_count,
              handler->GetCountForEventType(
                  FocusChangeEvent::activation_changing_event_type()));
    EXPECT_EQ(expected_changed_event_count,
              handler->GetCountForEventType(
                  FocusChangeEvent::activation_changed_event_type()));
  }

  TestFocusRules* test_focus_rules() { return test_focus_rules_; }

  // Test functions.
  virtual void BasicFocus() = 0;
  virtual void BasicActivation() = 0;
  virtual void FocusEvents() = 0;
  virtual void DuplicateFocusEvents() {}
  virtual void ActivationEvents() = 0;
  virtual void DuplicateActivationEvents() {}
  virtual void ShiftFocusWithinActiveWindow() {}
  virtual void ShiftFocusToChildOfInactiveWindow() {}
  virtual void FocusRulesOverride() = 0;
  virtual void ActivationRulesOverride() = 0;

 private:
  scoped_ptr<FocusController> focus_controller_;
  TestFocusRules* test_focus_rules_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerTestBase);
};

// Test base for tests where focus is directly set to a target window.
class FocusControllerDirectTestBase : public FocusControllerTestBase {
 protected:
  FocusControllerDirectTestBase() {}

  // Different test types shift focus in different ways.
  virtual void FocusWindowDirect(aura::Window* window) = 0;
  virtual void ActivateWindowDirect(aura::Window* window) = 0;
  virtual void DeactivateWindowDirect(aura::Window* window) = 0;

  void FocusWindowById(int id) {
    aura::Window* window = root_window()->GetChildById(id);
    DCHECK(window);
    FocusWindowDirect(window);
  }
  void ActivateWindowById(int id) {
    aura::Window* window = root_window()->GetChildById(id);
    DCHECK(window);
    ActivateWindowDirect(window);
  }

  // Overridden from FocusControllerTestBase:
  virtual void BasicFocus() OVERRIDE {
    EXPECT_EQ(NULL, focused_window());
    FocusWindowById(1);
    EXPECT_EQ(1, focused_window_id());
    FocusWindowById(2);
    EXPECT_EQ(2, focused_window_id());
  }
  virtual void BasicActivation() OVERRIDE {
    EXPECT_EQ(NULL, GetActiveWindow());
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    DeactivateWindow(GetActiveWindow());
    EXPECT_EQ(3, GetActiveWindowId());
  }
  virtual void FocusEvents() OVERRIDE {
    FocusEventsTestHandler handler(root_window()->GetChildById(1));
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
    FocusWindowById(1);
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
  }
  virtual void DuplicateFocusEvents() OVERRIDE {
    // Focusing an existing focused window should not resend focus events.
    FocusEventsTestHandler handler(root_window());
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
    FocusWindowById(1);
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
    FocusWindowById(1);
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
  }
  virtual void ActivationEvents() OVERRIDE {
    ActivateWindowById(1);

    FocusEventsTestHandler handler_root(root_window());
    FocusEventsTestHandler handler_1(root_window()->GetChildById(1));
    FocusEventsTestHandler handler_2(root_window()->GetChildById(2));

    ExpectActivationEvents(&handler_root, 0, 0);
    ExpectActivationEvents(&handler_1, 0, 0);
    ExpectActivationEvents(&handler_2, 0, 0);
    ActivateWindowById(2);
    ExpectActivationEvents(&handler_root, 1, 1);
    ExpectActivationEvents(&handler_1, 1, 0);
    ExpectActivationEvents(&handler_2, 0, 1);
  }
  virtual void DuplicateActivationEvents() OVERRIDE {
    // Activating an existing active window should not resend activation events.
    ActivateWindowById(1);

    FocusEventsTestHandler handler_root(root_window());
    ExpectActivationEvents(&handler_root, 0, 0);
    ActivateWindowById(2);
    ExpectActivationEvents(&handler_root, 1, 1);
    ActivateWindowById(2);
    ExpectActivationEvents(&handler_root, 1, 1);
  }
  virtual void ShiftFocusWithinActiveWindow() OVERRIDE {
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, focused_window_id());
    FocusWindowById(11);
    EXPECT_EQ(11, focused_window_id());
    FocusWindowById(12);
    EXPECT_EQ(12, focused_window_id());
  }
  virtual void ShiftFocusToChildOfInactiveWindow() OVERRIDE {
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, focused_window_id());
    FocusWindowById(11);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(11, focused_window_id());
  }
  virtual void FocusRulesOverride() OVERRIDE {
    EXPECT_EQ(NULL, focused_window());
    FocusWindowById(11);
    EXPECT_EQ(11, focused_window_id());

    test_focus_rules()->set_focus_restriction(root_window()->GetChildById(211));
    FocusWindowById(12);
    EXPECT_EQ(211, focused_window_id());

    test_focus_rules()->set_focus_restriction(NULL);
    FocusWindowById(12);
    EXPECT_EQ(12, focused_window_id());
  }
  virtual void ActivationRulesOverride() OVERRIDE {
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, focused_window_id());

    aura::Window* w3 = root_window()->GetChildById(3);
    test_focus_rules()->set_focus_restriction(w3);

    ActivateWindowById(2);
    // FocusRules restricts focus and activation to 3.
    EXPECT_EQ(3, GetActiveWindowId());
    EXPECT_EQ(3, focused_window_id());

    test_focus_rules()->set_focus_restriction(NULL);
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, focused_window_id());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerDirectTestBase);
};

// Focus and Activation changes via aura::client::ActivationClient API.
class FocusControllerApiTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerApiTest() {}

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    focus_controller()->FocusWindow(window);
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    ActivateWindow(window);
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    DeactivateWindow(window);
  }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerApiTest);
};

// Focus and Activation changes via input events.
class FocusControllerMouseEventTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerMouseEventTest() {}

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.ClickLeftButton();
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.ClickLeftButton();
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::Window* next_activatable =
        test_focus_rules()->GetNextActivatableWindow(window);
    aura::test::EventGenerator generator(root_window(), next_activatable);
    generator.ClickLeftButton();
  }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerMouseEventTest);
};

class FocusControllerGestureEventTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerGestureEventTest() {}

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::Window* next_activatable =
        test_focus_rules()->GetNextActivatableWindow(window);
    aura::test::EventGenerator generator(root_window(), next_activatable);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerGestureEventTest);
};

// Test base for tests where focus is implicitly set to a window as the result
// of a disposition change to the focused window or the hierarchy that contains
// it.
class FocusControllerImplicitTestBase : public FocusControllerTestBase {
 protected:
  explicit FocusControllerImplicitTestBase(bool parent) : parent_(parent) {}

  aura::Window* GetDispositionWindow(aura::Window* window) {
    return parent_ ? window->parent() : window;
  }

  // Change the disposition of |window| in such a way as it will lose focus.
  virtual void ChangeWindowDisposition(aura::Window* window) = 0;

  // Overridden from FocusControllerTestBase:
  virtual void BasicFocus() OVERRIDE {
    EXPECT_EQ(NULL, focused_window());

    aura::Window* w211 = root_window()->GetChildById(211);
    focus_controller()->FocusWindow(w211);
    EXPECT_EQ(211, focused_window_id());

    ChangeWindowDisposition(w211);
    // BasicFocusRules passes focus to the parent.
    EXPECT_EQ(parent_ ? 2 : 21, focused_window_id());
  }
  virtual void BasicActivation() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    EXPECT_EQ(NULL, GetActiveWindow());

    aura::Window* w2 = root_window()->GetChildById(2);
    ActivateWindow(w2);
    EXPECT_EQ(2, GetActiveWindowId());

    ChangeWindowDisposition(w2);
    EXPECT_EQ(3, GetActiveWindowId());
  }
  virtual void FocusEvents() OVERRIDE {
    aura::Window* w211 = root_window()->GetChildById(211);
    focus_controller()->FocusWindow(w211);

    FocusEventsTestHandler handler(root_window()->GetChildById(211));
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(0, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
    ChangeWindowDisposition(w211);
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changing_event_type()));
    EXPECT_EQ(1, handler.GetCountForEventType(
        FocusChangeEvent::focus_changed_event_type()));
  }
  virtual void ActivationEvents() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    aura::Window* w2 = root_window()->GetChildById(2);
    ActivateWindow(w2);

    FocusEventsTestHandler handler_root(root_window());
    FocusEventsTestHandler handler_2(root_window()->GetChildById(2));
    FocusEventsTestHandler handler_3(root_window()->GetChildById(3));

    ExpectActivationEvents(&handler_root, 0, 0);
    ExpectActivationEvents(&handler_2, 0, 0);
    ExpectActivationEvents(&handler_3, 0, 0);

    ChangeWindowDisposition(w2);
    ExpectActivationEvents(&handler_root, 1, 1);
    ExpectActivationEvents(&handler_2, 1, 0);
    ExpectActivationEvents(&handler_3, 0, 1);
  }
  virtual void FocusRulesOverride() OVERRIDE {
    EXPECT_EQ(NULL, focused_window());
    aura::Window* w211 = root_window()->GetChildById(211);
    focus_controller()->FocusWindow(w211);
    EXPECT_EQ(211, focused_window_id());

    test_focus_rules()->set_focus_restriction(root_window()->GetChildById(11));
    ChangeWindowDisposition(w211);
    // Normally, focus would shift to the parent (w21) but the override shifts
    // it to 11.
    EXPECT_EQ(11, focused_window_id());

    test_focus_rules()->set_focus_restriction(NULL);
  }
  virtual void ActivationRulesOverride() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    aura::Window* w1 = root_window()->GetChildById(1);
    ActivateWindow(w1);

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, focused_window_id());

    aura::Window* w3 = root_window()->GetChildById(3);
    test_focus_rules()->set_focus_restriction(w3);

    // Normally, activation/focus would move to w2, but since we have a focus
    // restriction, it should move to w3 instead.
    ChangeWindowDisposition(w1);
    EXPECT_EQ(3, GetActiveWindowId());
    EXPECT_EQ(3, focused_window_id());

    test_focus_rules()->set_focus_restriction(NULL);
    ActivateWindow(root_window()->GetChildById(2));
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, focused_window_id());
  }

 private:
  // When true, the disposition change occurs to the parent of the window
  // instead of to the window. This verifies that changes occurring in the
  // hierarchy that contains the window affect the window's focus.
  bool parent_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerImplicitTestBase);
};

// Focus and Activation changes in response to window visibility changes.
class FocusControllerHideTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerHideTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerHideTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    GetDispositionWindow(window)->Hide();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerHideTest);
};

// Focus and Activation changes in response to window parent visibility
// changes.
class FocusControllerParentHideTest : public FocusControllerHideTest {
 public:
  FocusControllerParentHideTest() : FocusControllerHideTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentHideTest);
};

// Focus and Activation changes in response to window destruction.
class FocusControllerDestructionTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerDestructionTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerDestructionTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    delete GetDispositionWindow(window);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerDestructionTest);
};

// Focus and Activation changes in response to window parent destruction.
class FocusControllerParentDestructionTest
    : public FocusControllerDestructionTest {
 public:
  FocusControllerParentDestructionTest()
      : FocusControllerDestructionTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentDestructionTest);
};

// Focus and Activation changes in response to window removal.
class FocusControllerRemovalTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerRemovalTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerRemovalTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    aura::Window* disposition_window = GetDispositionWindow(window);
    disposition_window->parent()->RemoveChild(disposition_window);
    window_owner_.reset(disposition_window);
  }
  virtual void TearDown() OVERRIDE {
    window_owner_.reset();
    FocusControllerImplicitTestBase::TearDown();
  }

 private:
  scoped_ptr<aura::Window> window_owner_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerRemovalTest);
};

// Focus and Activation changes in response to window parent removal.
class FocusControllerParentRemovalTest : public FocusControllerRemovalTest {
 public:
  FocusControllerParentRemovalTest() : FocusControllerRemovalTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentRemovalTest);
};


#define FOCUS_CONTROLLER_TEST(TESTCLASS, TESTNAME) \
    TEST_F(TESTCLASS, TESTNAME) { TESTNAME(); }

// Runs direct focus change tests (input events and API calls).
#define DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerApiTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerMouseEventTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerGestureEventTest, TESTNAME)

// Runs implicit focus change tests for disposition changes to target.
#define IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerHideTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerDestructionTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerRemovalTest, TESTNAME)

// Runs implicit focus change tests for disposition changes to target's parent
// hierarchy.
#define IMPLICIT_FOCUS_CHANGE_PARENT_TESTS(TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerParentHideTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerParentDestructionTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerParentRemovalTest, TESTNAME)

// Runs all implicit focus change tests (changes to the target and target's
// parent hierarchy)
#define IMPLICIT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_PARENT_TESTS(TESTNAME)

// Runs all possible focus change tests.
#define ALL_FOCUS_TESTS(TESTNAME) \
    DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TESTS(TESTNAME)

// Runs focus change tests that apply only to the target. For example,
// implicit activation changes caused by window disposition changes do not
// occur when changes to the containing hierarchy happen.
#define TARGET_FOCUS_TESTS(TESTNAME) \
    DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME)

// - Focuses a window, verifies that focus changed.
ALL_FOCUS_TESTS(BasicFocus);

// - Activates a window, verifies that activation changed.
TARGET_FOCUS_TESTS(BasicActivation);

// - Focuses a window, verifies that focus events were dispatched.
ALL_FOCUS_TESTS(FocusEvents);

// - Focuses or activates a window multiple times, verifies that events are only
//   dispatched when focus/activation actually changes.
DIRECT_FOCUS_CHANGE_TESTS(DuplicateFocusEvents);
DIRECT_FOCUS_CHANGE_TESTS(DuplicateActivationEvents);

// - Activates a window, verifies that activation events were dispatched.
TARGET_FOCUS_TESTS(ActivationEvents);

// - Input events/API calls shift focus between focusable windows within the
//   active window.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusWithinActiveWindow);

// - Input events/API calls to a child window of an inactive window shifts
//   activation to the activatable parent and focuses the child.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusToChildOfInactiveWindow);

// - Verifies that FocusRules determine what can be focused.
ALL_FOCUS_TESTS(FocusRulesOverride);

// - Verifies that FocusRules determine what can be activated.
TARGET_FOCUS_TESTS(ActivationRulesOverride);

}  // namespace corewm
}  // namespace views
