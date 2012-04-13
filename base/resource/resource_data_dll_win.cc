// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_data_dll_win.h"

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/win/resource_util.h"

namespace ui {

ResourceDataDLL::ResourceDataDLL(HINSTANCE module) : module_(module) {
  DCHECK(module_);
}

ResourceDataDLL::~ResourceDataDLL() {
}

bool ResourceDataDLL::GetStringPiece(uint16 resource_id,
                                     base::StringPiece* data) const {
  DCHECK(data);
  void* data_ptr;
  size_t data_size;
  if (base::win::GetDataResourceFromModule(module_,
                                           resource_id,
                                           &data_ptr,
                                           &data_size)) {
    data->set(static_cast<const char*>(data_ptr), data_size);
    return true;
  }
  return false;
}

RefCountedStaticMemory* ResourceDataDLL::GetStaticMemory(
    uint16 resource_id) const {
  void* data_ptr;
  size_t data_size;
  if (base::win::GetDataResourceFromModule(module_, resource_id, &data_ptr,
                                           &data_size)) {
    return new RefCountedStaticMemory(
        reinterpret_cast<const unsigned char*>(data_ptr), data_size);
  }
  return NULL;
}

ResourceHandle::TextEncodingType ResourceDataDLL::GetTextEncodingType() const {
  return BINARY;
}

}  // namespace ui