// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_WIN_H_

#include <windows.foundation.h>
#include <windows.graphics.imaging.h>
#include <memory>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/face_detection_impl_win.h"
#include "services/shape_detection/public/mojom/facedetection_provider.mojom.h"

namespace shape_detection {

class FaceDetectionProviderWin
    : public shape_detection::mojom::FaceDetectionProvider {
 public:
  using FaceDetector = ABI::Windows::Media::FaceAnalysis::FaceDetector;
  using IFaceDetector = ABI::Windows::Media::FaceAnalysis::IFaceDetector;
  using BitmapPixelFormat = ABI::Windows::Graphics::Imaging::BitmapPixelFormat;

  FaceDetectionProviderWin();
  ~FaceDetectionProviderWin() override;

  static void Create(
      shape_detection::mojom::FaceDetectionProviderRequest request) {
    auto provider = std::make_unique<FaceDetectionProviderWin>();
    auto* provider_ptr = provider.get();
    provider_ptr->binding_ =
        mojo::MakeStrongBinding(std::move(provider), std::move(request));
  }

  void CreateFaceDetection(
      shape_detection::mojom::FaceDetectionRequest request,
      shape_detection::mojom::FaceDetectorOptionsPtr options) override;

 private:
  void OnFaceDetectorCreated(
      shape_detection::mojom::FaceDetectionRequest request,
      BitmapPixelFormat pixel_format,
      AsyncOperation<FaceDetector>::IAsyncOperationPtr async_op);

  FRIEND_TEST_ALL_PREFIXES(FaceDetectionImplWinTest, ScanOneFace);
  mojo::StrongBindingPtr<mojom::FaceDetectionProvider> binding_;
  std::unique_ptr<AsyncOperation<FaceDetector>> async_create_detector_ops_;

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionProviderWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_WIN_H_
