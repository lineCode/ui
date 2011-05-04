// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/image.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "third_party/skia/include/core/SkBitmap.h"

#if defined(OS_LINUX)
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include "ui/gfx/canvas_skia.h"
#include "ui/gfx/gtk_util.h"
#elif defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#include "skia/ext/skia_utils_mac.h"
#endif

namespace gfx {

namespace internal {

#if defined(OS_MACOSX)
// This is a wrapper around gfx::NSImageToSkBitmap() because this cross-platform
// file cannot include the [square brackets] of ObjC.
bool NSImageToSkBitmaps(NSImage* image, std::vector<const SkBitmap*>* bitmaps);
#endif

#if defined(OS_LINUX)
const SkBitmap* GdkPixbufToSkBitmap(GdkPixbuf* pixbuf) {
  gfx::CanvasSkia canvas(gdk_pixbuf_get_width(pixbuf),
                         gdk_pixbuf_get_height(pixbuf),
                         /*is_opaque=*/false);
  canvas.DrawGdkPixbuf(pixbuf, 0, 0);
  return new SkBitmap(canvas.ExtractBitmap());
}
#endif

class SkBitmapRep;
class GdkPixbufRep;
class NSImageRep;

// An ImageRep is the object that holds the backing memory for an Image. Each
// RepresentationType has an ImageRep subclass that is responsible for freeing
// the memory that the ImageRep holds. When an ImageRep is created, it expects
// to take ownership of the image, without having to retain it or increase its
// reference count.
class ImageRep {
 public:
  explicit ImageRep(Image::RepresentationType rep) : type_(rep) {}

  // Deletes the associated pixels of an ImageRep.
  virtual ~ImageRep() {}

  // Cast helpers ("fake RTTI").
  SkBitmapRep* AsSkBitmapRep() {
    CHECK_EQ(type_, Image::kSkBitmapRep);
    return reinterpret_cast<SkBitmapRep*>(this);
  }

#if defined(OS_LINUX)
  GdkPixbufRep* AsGdkPixbufRep() {
    CHECK_EQ(type_, Image::kGdkPixbufRep);
    return reinterpret_cast<GdkPixbufRep*>(this);
  }
#endif

#if defined(OS_MACOSX)
  NSImageRep* AsNSImageRep() {
    CHECK_EQ(type_, Image::kNSImageRep);
    return reinterpret_cast<NSImageRep*>(this);
  }
#endif

  Image::RepresentationType type() const { return type_; }

 private:
  Image::RepresentationType type_;
};

class SkBitmapRep : public ImageRep {
 public:
  explicit SkBitmapRep(const SkBitmap* bitmap)
      : ImageRep(Image::kSkBitmapRep) {
    CHECK(bitmap);
    bitmaps_.push_back(bitmap);
  }

  explicit SkBitmapRep(const std::vector<const SkBitmap*>& bitmaps)
      : ImageRep(Image::kSkBitmapRep),
        bitmaps_(bitmaps) {
    CHECK(!bitmaps_.empty());
  }

  virtual ~SkBitmapRep() {
    STLDeleteElements(&bitmaps_);
  }

  const SkBitmap* bitmap() const { return bitmaps_[0]; }

  const std::vector<const SkBitmap*>& bitmaps() const { return bitmaps_; }

 private:
  std::vector<const SkBitmap*> bitmaps_;

  DISALLOW_COPY_AND_ASSIGN(SkBitmapRep);
};

#if defined(OS_LINUX)
class GdkPixbufRep : public ImageRep {
 public:
  explicit GdkPixbufRep(GdkPixbuf* pixbuf)
      : ImageRep(Image::kGdkPixbufRep),
        pixbuf_(pixbuf) {
    CHECK(pixbuf);
  }

  virtual ~GdkPixbufRep() {
    if (pixbuf_) {
      g_object_unref(pixbuf_);
      pixbuf_ = NULL;
    }
  }

  GdkPixbuf* pixbuf() const { return pixbuf_; }

 private:
  GdkPixbuf* pixbuf_;

  DISALLOW_COPY_AND_ASSIGN(GdkPixbufRep);
};
#endif

#if defined(OS_MACOSX)
class NSImageRep : public ImageRep {
 public:
  explicit NSImageRep(NSImage* image)
      : ImageRep(Image::kNSImageRep),
        image_(image) {
    CHECK(image);
  }

  virtual ~NSImageRep() {
    base::mac::NSObjectRelease(image_);
    image_ = nil;
  }

  NSImage* image() const { return image_; }

 private:
  NSImage* image_;

  DISALLOW_COPY_AND_ASSIGN(NSImageRep);
};
#endif

// The Storage class acts similarly to the pixels in a SkBitmap: the Image
// class holds a refptr instance of Storage, which in turn holds all the
// ImageReps. This way, the Image can be cheaply copied.
class ImageStorage : public base::RefCounted<ImageStorage> {
 public:
  ImageStorage(gfx::Image::RepresentationType default_type)
      : default_representation_type_(default_type) {
  }

  gfx::Image::RepresentationType default_representation_type() {
    return default_representation_type_;
  }
  gfx::Image::RepresentationMap& representations() { return representations_; }

 private:
  ~ImageStorage() {
    for (gfx::Image::RepresentationMap::iterator it = representations_.begin();
         it != representations_.end();
         ++it) {
      delete it->second;
    }
    representations_.clear();
  }

  // The type of image that was passed to the constructor. This key will always
  // exist in the |representations_| map.
  gfx::Image::RepresentationType default_representation_type_;

  // All the representations of an Image. Size will always be at least one, with
  // more for any converted representations.
  gfx::Image::RepresentationMap representations_;

  friend class base::RefCounted<ImageStorage>;
};

}  // namespace internal

Image::Image(const SkBitmap* bitmap)
    : storage_(new internal::ImageStorage(Image::kSkBitmapRep)) {
  internal::SkBitmapRep* rep = new internal::SkBitmapRep(bitmap);
  AddRepresentation(rep);
}

Image::Image(const std::vector<const SkBitmap*>& bitmaps)
    : storage_(new internal::ImageStorage(Image::kSkBitmapRep)) {
  internal::SkBitmapRep* rep = new internal::SkBitmapRep(bitmaps);
  AddRepresentation(rep);
}

#if defined(OS_LINUX)
Image::Image(GdkPixbuf* pixbuf)
    : storage_(new internal::ImageStorage(Image::kGdkPixbufRep)) {
  internal::GdkPixbufRep* rep = new internal::GdkPixbufRep(pixbuf);
  AddRepresentation(rep);
}
#endif

#if defined(OS_MACOSX)
Image::Image(NSImage* image)
    : storage_(new internal::ImageStorage(Image::kNSImageRep)) {
  internal::NSImageRep* rep = new internal::NSImageRep(image);
  AddRepresentation(rep);
}
#endif

Image::Image(const Image& other) : storage_(other.storage_) {
}

Image& Image::operator=(const Image& other) {
  storage_ = other.storage_;
  return *this;
}

Image::~Image() {
}

Image::operator const SkBitmap*() const {
  internal::ImageRep* rep = GetRepresentation(Image::kSkBitmapRep);
  return rep->AsSkBitmapRep()->bitmap();
}

Image::operator const SkBitmap&() const {
  return *(this->operator const SkBitmap*());
}

#if defined(OS_LINUX)
Image::operator GdkPixbuf*() const {
  internal::ImageRep* rep = GetRepresentation(Image::kGdkPixbufRep);
  return rep->AsGdkPixbufRep()->pixbuf();
}
#endif

#if defined(OS_MACOSX)
Image::operator NSImage*() const {
  internal::ImageRep* rep = GetRepresentation(Image::kNSImageRep);
  return rep->AsNSImageRep()->image();
}
#endif

bool Image::HasRepresentation(RepresentationType type) const {
  return storage_->representations().count(type) != 0;
}

size_t Image::RepresentationCount() const {
  return storage_->representations().size();
}

void Image::SwapRepresentations(gfx::Image* other) {
  storage_.swap(other->storage_);
}

internal::ImageRep* Image::DefaultRepresentation() const {
  RepresentationMap& representations = storage_->representations();
  RepresentationMap::iterator it =
      representations.find(storage_->default_representation_type());
  DCHECK(it != representations.end());
  return it->second;
}

internal::ImageRep* Image::GetRepresentation(
    RepresentationType rep_type) const {
  // If the requested rep is the default, return it.
  internal::ImageRep* default_rep = DefaultRepresentation();
  if (rep_type == storage_->default_representation_type())
    return default_rep;

  // Check to see if the representation already exists.
  RepresentationMap::iterator it = storage_->representations().find(rep_type);
  if (it != storage_->representations().end())
    return it->second;

  // At this point, the requested rep does not exist, so it must be converted
  // from the default rep.

  // Handle native-to-Skia conversion.
  if (rep_type == Image::kSkBitmapRep) {
    internal::SkBitmapRep* rep = NULL;
#if defined(OS_LINUX)
    if (storage_->default_representation_type() == Image::kGdkPixbufRep) {
      internal::GdkPixbufRep* pixbuf_rep = default_rep->AsGdkPixbufRep();
      rep = new internal::SkBitmapRep(
          internal::GdkPixbufToSkBitmap(pixbuf_rep->pixbuf()));
    }
#elif defined(OS_MACOSX)
    if (storage_->default_representation_type() == Image::kNSImageRep) {
      internal::NSImageRep* nsimage_rep = default_rep->AsNSImageRep();
      std::vector<const SkBitmap*> bitmaps;
      CHECK(internal::NSImageToSkBitmaps(nsimage_rep->image(), &bitmaps));
      rep = new internal::SkBitmapRep(bitmaps);
    }
#endif
    CHECK(rep);
    AddRepresentation(rep);
    return rep;
  }

  // Handle Skia-to-native conversions.
  if (default_rep->type() == Image::kSkBitmapRep) {
    internal::SkBitmapRep* skia_rep = default_rep->AsSkBitmapRep();
    internal::ImageRep* native_rep = NULL;
#if defined(OS_LINUX)
    if (rep_type == Image::kGdkPixbufRep) {
      GdkPixbuf* pixbuf = gfx::GdkPixbufFromSkBitmap(skia_rep->bitmap());
      native_rep = new internal::GdkPixbufRep(pixbuf);
    }
#elif defined(OS_MACOSX)
    if (rep_type == Image::kNSImageRep) {
      NSImage* image = gfx::SkBitmapsToNSImage(skia_rep->bitmaps());
      base::mac::NSObjectRetain(image);
      native_rep = new internal::NSImageRep(image);
    }
#endif
    CHECK(native_rep);
    AddRepresentation(native_rep);
    return native_rep;
  }

  // Something went seriously wrong...
  return NULL;
}

void Image::AddRepresentation(internal::ImageRep* rep) const {
  storage_->representations().insert(std::make_pair(rep->type(), rep));
}

size_t Image::GetNumberOfSkBitmaps() const  {
  return GetRepresentation(Image::kSkBitmapRep)->AsSkBitmapRep()->
      bitmaps().size();
}

const SkBitmap* Image::GetSkBitmapAtIndex(size_t index) const {
  return GetRepresentation(Image::kSkBitmapRep)->AsSkBitmapRep()->
      bitmaps()[index];
}

}  // namespace gfx
