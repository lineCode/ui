// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ui_base_switches.h"

namespace switches {

// The language file that we want to try to open. Of the form
// language[-country] where language is the 2 letter code from ISO-639.
const char kLang[]                          = "lang";

// Load the locale resources from the given path. When running on Mac/Unix the
// path should point to a locale.pak file.
const char kLocalePak[]                     = "locale_pak";

// Disable ui::MessageBox. This is useful when running as part of scripts that
// do not have a user interface.
const char kNoMessageBox[]                  = "no-message-box";

// Enables UI changes that make it easier to use with a touchscreen.
const char kTouchOptimizedUI[]              = "touch-optimized-ui";

#if defined(OS_MACOSX)
const char kDisableCompositedCoreAnimationPlugins[] =
    "disable-composited-core-animation-plugins";
#endif

}  // namespace switches
