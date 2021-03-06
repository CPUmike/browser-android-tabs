// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_MOJO_IMAGE_CAPTURE_TYPES_H_
#define MEDIA_CAPTURE_MOJO_IMAGE_CAPTURE_TYPES_H_

#include "media/capture/mojo/image_capture.mojom.h"

namespace mojo {

media::mojom::PhotoStatePtr CreateEmptyPhotoState();

}  // namespace mojo

#endif  // MEDIA_CAPTURE_MOJO_IMAGE_CAPTURE_TYPES_H_