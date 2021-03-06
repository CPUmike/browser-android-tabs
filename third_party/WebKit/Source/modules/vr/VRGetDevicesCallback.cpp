// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/VRGetDevicesCallback.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"

namespace blink {

VRGetDevicesCallback::VRGetDevicesCallback(ScriptPromiseResolver* resolver)
    : resolver_(resolver) {}

VRGetDevicesCallback::~VRGetDevicesCallback() = default;

void VRGetDevicesCallback::OnSuccess(VRDisplayVector displays) {
  bool display_supports_presentation = false;
  for (auto display : displays) {
    if (display->capabilities()->canPresent()) {
      display_supports_presentation = true;
    }
  }

  if (display_supports_presentation) {
    ExecutionContext* execution_context =
        ExecutionContext::From(resolver_->GetScriptState());
    UseCounter::Count(execution_context,
                      WebFeature::kVRGetDisplaysSupportsPresent);
  }

  resolver_->Resolve(displays);
}

void VRGetDevicesCallback::OnError() {
  resolver_->Reject();
}

}  // namespace blink
