// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_IMAGE_IMAGE_SKIA_H_
#define UI_GFX_IMAGE_IMAGE_SKIA_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "ui/base/ui_export.h"
#include "ui/gfx/image/image_skia_rep.h"

namespace gfx {
class ImageSkiaSource;
class Size;

namespace internal {
class ImageSkiaStorage;
}  // namespace internal

// Container for the same image at different densities, similar to NSImage.
// Image height and width are in DIP (Density Indepent Pixel) coordinates.
//
// ImageSkia should be used whenever possible instead of SkBitmap.
// Functions that mutate the image should operate on the gfx::ImageSkiaRep
// returned from ImageSkia::GetRepresentation, not on ImageSkia.
//
// ImageSkia is cheap to copy and intentionally supports copy semantics.
class UI_EXPORT ImageSkia {
 public:
  typedef std::vector<ImageSkiaRep> ImageSkiaReps;

  // Creates an instance with no bitmaps.
  ImageSkia();

  // Creates an instance that will use the |source| to get the image
  // for scale factors. |size| specifes the size of the image in DIP.
  ImageSkia(ImageSkiaSource* source, const gfx::Size& size);

  // Adds ref to passed in bitmap.
  // DIP width and height are set based on scale factor of 1x.
  // TODO(pkotwicz): This is temporary till conversion to gfx::ImageSkia is
  // done.
  ImageSkia(const SkBitmap& bitmap);

  ImageSkia(const gfx::ImageSkiaRep& image_rep);

  // Copies a reference to |other|'s storage.
  ImageSkia(const ImageSkia& other);

  // Copies a reference to |other|'s storage.
  ImageSkia& operator=(const ImageSkia& other);

  // Converts from SkBitmap.
  // Adds ref to passed in bitmap.
  // DIP width and height are set based on scale factor of 1x.
  // TODO(pkotwicz): This is temporary till conversion to gfx::ImageSkia is
  // done.
  ImageSkia& operator=(const SkBitmap& other);

  // Converts to gfx::ImageSkiaRep and SkBitmap.
  // TODO(pkotwicz): This is temporary till conversion to gfx::ImageSkia is
  // done.
  operator SkBitmap&() const;

  ~ImageSkia();

  // Adds |image_rep| to the image reps contained by this object.
  void AddRepresentation(const gfx::ImageSkiaRep& image_rep);

  // Removes the image rep of |scale_factor| if present.
  void RemoveRepresentation(ui::ScaleFactor scale_factor);

  // Returns true if the object owns a image rep whose density matches
  // |scale_factor| exactly.
  bool HasRepresentation(ui::ScaleFactor scale_factor);

  // Returns the image rep whose density best matches
  // |scale_factor|.
  // Returns a null image rep if the object contains no image reps.
  const gfx::ImageSkiaRep& GetRepresentation(
      ui::ScaleFactor scale_factor) const;

  // Returns true if object is null or its size is empty.
  bool empty() const;

  // Returns true if this is a null object.
  // TODO(pkotwicz): Merge this function into empty().
  bool isNull() const { return storage_ == NULL; }

  // Width and height of image in DIP coordinate system.
  int width() const;
  int height() const;
  gfx::Size size() const;

  // Wrapper function for SkBitmap::extractBitmap.
  // Operates on each stored image rep. Note that it may not have
  // all image reps for supported scale factors.
  // TODO(oshima|pkotwicz): Investigate if this can be eliminated
  // after ImageSkiaSource conversion.
  bool extractSubset(ImageSkia* dst, const SkIRect& subset) const;

  // Returns pointer to an SkBitmap contained by this object.
  // TODO(pkotwicz): This is temporary till conversion to gfx::ImageSkia is
  // done.
  const SkBitmap* bitmap() const;

  // Returns a vector with the image reps contained in this object.
  // There is no guarantee that this will return all images rep for
  // supported scale factors.
  // TODO(oshima): Update all use of this API and make this to fail
  // when source is used.
  std::vector<gfx::ImageSkiaRep> image_reps() const;

 private:
  // Initialize ImageSkiaStorage with passed in parameters.
  // If the image rep's bitmap is empty, ImageStorage is set to NULL.
  void Init(const gfx::ImageSkiaRep& image_rep);

  // A refptr so that ImageRepSkia can be copied cheaply.
  scoped_refptr<internal::ImageSkiaStorage> storage_;
};

}  // namespace gfx

#endif  // UI_GFX_IMAGE_IMAGE_SKIA_H_
