// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/size_f.h"

#include "base/stringprintf.h"
#include "ui/gfx/size_base_impl.h"

namespace gfx {

template class SizeBase<SizeF, float>;

SizeF::SizeF() : SizeBase<SizeF, float>(0, 0) {}

SizeF::SizeF(float width, float height)
    : SizeBase<SizeF, float>(width, height) {
}

SizeF::~SizeF() {}

std::string SizeF::ToString() const {
  return base::StringPrintf("%fx%f", width(), height());
}

}  // namespace gfx
