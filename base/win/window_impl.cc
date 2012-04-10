// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/window_impl.h"

#include <list>

#include "base/debug/alias.h"
#include "base/memory/singleton.h"
#include "base/string_number_conversions.h"
#include "base/win/wrapped_window_proc.h"
#include "ui/base/win/hwnd_util.h"

namespace {

extern "C" {
  typedef HWND (*GetRootWindow)();
}

HMODULE GetMetroDll() {
  static HMODULE hm = ::GetModuleHandleA("metro_driver.dll");
  return hm;
}

HWND RootWindow(bool is_child_window) {
  HMODULE metro = GetMetroDll();
  if (!metro) {
    return is_child_window ? ::GetDesktopWindow() : HWND_DESKTOP;
  }
  GetRootWindow get_root_window =
      reinterpret_cast<GetRootWindow>(::GetProcAddress(metro, "GetRootWindow"));
  return get_root_window();
}

}  // namespace

namespace ui {

static const DWORD kWindowDefaultChildStyle =
    WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
static const DWORD kWindowDefaultStyle = WS_OVERLAPPEDWINDOW;
static const DWORD kWindowDefaultExStyle = 0;

///////////////////////////////////////////////////////////////////////////////
// WindowImpl class tracking.

// Several external scripts rely explicitly on this base class name for
// acquiring the window handle and will break if this is modified!
// static
const wchar_t* const WindowImpl::kBaseClassName = L"Chrome_WidgetWin_";

// WindowImpl class information used for registering unique windows.
struct ClassInfo {
  UINT style;
  HICON icon;

  ClassInfo(int style, HICON icon)
      : style(style),
        icon(icon) {}

  // Compares two ClassInfos. Returns true if all members match.
  bool Equals(const ClassInfo& other) const {
    return (other.style == style && other.icon == icon);
  }
};

class ClassRegistrar {
 public:
  static ClassRegistrar* GetInstance() {
    return Singleton<ClassRegistrar>::get();
  }

  ~ClassRegistrar() {
    for (RegisteredClasses::iterator i = registered_classes_.begin();
         i != registered_classes_.end(); ++i) {
      if (!UnregisterClass(i->name.c_str(), NULL)) {
        LOG(ERROR) << "Failed to unregister class " << i->name.c_str()
                   << ". Error = " << GetLastError();
      }
    }
  }

  // Puts the name for the class matching |class_info| in |class_name|, creating
  // a new name if the class is not yet known.
  // Returns true if this class was already known, false otherwise.
  bool RetrieveClassName(const ClassInfo& class_info, std::wstring* name) {
    for (RegisteredClasses::const_iterator i = registered_classes_.begin();
         i != registered_classes_.end(); ++i) {
      if (class_info.Equals(i->info)) {
        name->assign(i->name);
        return true;
      }
    }

    name->assign(string16(WindowImpl::kBaseClassName) +
        base::IntToString16(registered_count_++));
    return false;
  }

  void RegisterClass(const ClassInfo& class_info,
                     const std::wstring& name,
                     ATOM atom) {
    registered_classes_.push_back(RegisteredClass(class_info, name, atom));
  }

 private:
  // Represents a registered window class.
  struct RegisteredClass {
    RegisteredClass(const ClassInfo& info,
                    const std::wstring& name,
                    ATOM atom)
        : info(info),
          name(name),
          atom(atom) {
    }

    // Info used to create the class.
    ClassInfo info;

    // The name given to the window.
    std::wstring name;

    // The ATOM returned from creating the window.
    ATOM atom;
  };

  ClassRegistrar() : registered_count_(0) { }
  friend struct DefaultSingletonTraits<ClassRegistrar>;

  typedef std::list<RegisteredClass> RegisteredClasses;
  RegisteredClasses registered_classes_;

  // Counter of how many classes have been registered so far.
  int registered_count_;

  DISALLOW_COPY_AND_ASSIGN(ClassRegistrar);
};

///////////////////////////////////////////////////////////////////////////////
// WindowImpl, public

WindowImpl::WindowImpl()
    : window_style_(0),
      window_ex_style_(kWindowDefaultExStyle),
      class_style_(CS_DBLCLKS),
      hwnd_(NULL),
      got_create_(false),
      got_valid_hwnd_(false),
      destroyed_(NULL) {
}

WindowImpl::~WindowImpl() {
  if (destroyed_)
    *destroyed_ = true;
  if (::IsWindow(hwnd_))
    ui::SetWindowUserData(hwnd_, NULL);
}

void WindowImpl::Init(HWND parent, const gfx::Rect& bounds) {
  if (window_style_ == 0)
    window_style_ = parent ? kWindowDefaultChildStyle : kWindowDefaultStyle;

  if (parent == HWND_DESKTOP) {
    // Only non-child windows can have HWND_DESKTOP (0) as their parent.
    CHECK((window_style_ & WS_CHILD) == 0);
    parent = RootWindow(false);
  } else if (parent == ::GetDesktopWindow()) {
    // Any type of window can have the "Desktop Window" as their parent.
    parent = RootWindow(true);
  } else if (parent != HWND_MESSAGE) {
    CHECK(::IsWindow(parent));
  }

  int x, y, width, height;
  if (bounds.IsEmpty()) {
    x = y = width = height = CW_USEDEFAULT;
  } else {
    x = bounds.x();
    y = bounds.y();
    width = bounds.width();
    height = bounds.height();
  }

  std::wstring name(GetWindowClassName());
  bool destroyed = false;
  destroyed_ = &destroyed;
  HWND hwnd = CreateWindowEx(window_ex_style_, name.c_str(), NULL,
                             window_style_, x, y, width, height,
                             parent, NULL, NULL, this);
  if (!hwnd_) {
    base::debug::Alias(&destroyed);
    base::debug::Alias(&hwnd);
    DWORD last_error = GetLastError();
    base::debug::Alias(&last_error);
    bool got_create = got_create_;
    base::debug::Alias(&got_create);
    bool got_valid_hwnd = got_valid_hwnd_;
    base::debug::Alias(&got_valid_hwnd);
    CHECK(false);
  }
  if (!destroyed)
    destroyed_ = NULL;

  // The window procedure should have set the data for us.
  CHECK_EQ(this, ui::GetWindowUserData(hwnd));
}

HICON WindowImpl::GetDefaultWindowIcon() const {
  return NULL;
}

LRESULT WindowImpl::OnWndProc(UINT message, WPARAM w_param, LPARAM l_param) {
  LRESULT result = 0;

  // Handle the message if it's in our message map; otherwise, let the system
  // handle it.
  if (!ProcessWindowMessage(hwnd_, message, w_param, l_param, result))
    result = DefWindowProc(hwnd_, message, w_param, l_param);

  return result;
}

// static
LRESULT CALLBACK WindowImpl::WndProc(HWND hwnd,
                                     UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param) {
  if (message == WM_NCCREATE) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(l_param);
    WindowImpl* window = reinterpret_cast<WindowImpl*>(cs->lpCreateParams);
    DCHECK(window);
    ui::SetWindowUserData(hwnd, window);
    window->hwnd_ = hwnd;
    window->got_create_ = true;
    if (hwnd)
      window->got_valid_hwnd_ = true;
    return TRUE;
  }

  WindowImpl* window = reinterpret_cast<WindowImpl*>(
      ui::GetWindowUserData(hwnd));
  if (!window)
    return 0;

  return window->OnWndProc(message, w_param, l_param);
}

std::wstring WindowImpl::GetWindowClassName() {
  HICON icon = GetDefaultWindowIcon();
  ClassInfo class_info(initial_class_style(), icon);
  std::wstring name;
  if (ClassRegistrar::GetInstance()->RetrieveClassName(class_info, &name))
    return name;

  // No class found, need to register one.
  HBRUSH background = NULL;
  WNDCLASSEX class_ex = {
    sizeof(WNDCLASSEX),
    class_info.style,
    base::win::WrappedWindowProc<&WindowImpl::WndProc>,
    0,
    0,
    NULL,
    icon,
    NULL,
    reinterpret_cast<HBRUSH>(background + 1),
    NULL,
    name.c_str(),
    icon
  };
  ATOM atom = RegisterClassEx(&class_ex);
  CHECK(atom) << GetLastError();

  ClassRegistrar::GetInstance()->RegisterClass(class_info, name, atom);

  return name;
}

}  // namespace ui
