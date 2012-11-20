// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_NATIVE_THEME_COMMON_THEME_H_
#define UI_NATIVE_THEME_COMMON_THEME_H_

#include "ui/native_theme/native_theme.h"

class SkCanvas;
namespace views {
struct MenuConfig;
}

namespace ui {

// Drawing code that is common for all platforms.

// Returns true and |color| if |color_id| is found, or false otherwise.
bool CommonThemeGetSystemColor(NativeTheme::ColorId color_id, SkColor* color);

void CommonThemePaintMenuSeparator(
    SkCanvas* canvas,
    const gfx::Rect& rect,
    const NativeTheme::MenuSeparatorExtraParams& extra);

void CommonThemePaintMenuGutter(SkCanvas* canvas, const gfx::Rect& rect);

void CommonThemePaintMenuBackground(SkCanvas* canvas, const gfx::Rect& rect);

void CommonThemePaintMenuItemBackground(SkCanvas* canvas,
                                        NativeTheme::State state,
                                        const gfx::Rect& rect);

}  // namespace ui

#endif  // UI_NATIVE_THEME_COMMON_THEME_H_
