// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/string16.h"

namespace gfx {
class Rect;
}

namespace oak {
namespace internal {

// Functions that return a string consisting of a prefix and the supplied value
// converted to a pretty string representation.
string16 PropertyWithInteger(const std::string& prefix, int value);
string16 PropertyWithVoidStar(const std::string& prefix, void* ptr);
string16 PropertyWithBool(const std::string& prefix, bool value);
string16 PropertyWithBounds(const std::string& prefix, const gfx::Rect& bounds);

}  // namespace internal
}  // namespace oak
