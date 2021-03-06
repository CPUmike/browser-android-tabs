// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/DOMWindowPerformance.h"

#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/timing/WindowPerformance.h"

namespace blink {

DOMWindowPerformance::DOMWindowPerformance(LocalDOMWindow& window)
    : Supplement<LocalDOMWindow>(window) {}

void DOMWindowPerformance::Trace(blink::Visitor* visitor) {
  visitor->Trace(performance_);
  Supplement<LocalDOMWindow>::Trace(visitor);
}

void DOMWindowPerformance::TraceWrappers(
    const ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(performance_);
  Supplement<LocalDOMWindow>::TraceWrappers(visitor);
}

// static
const char DOMWindowPerformance::kSupplementName[] = "DOMWindowPerformance";

// static
DOMWindowPerformance& DOMWindowPerformance::From(LocalDOMWindow& window) {
  DOMWindowPerformance* supplement =
      Supplement<LocalDOMWindow>::From<DOMWindowPerformance>(window);
  if (!supplement) {
    supplement = new DOMWindowPerformance(window);
    ProvideTo(window, supplement);
  }
  return *supplement;
}

// static
WindowPerformance* DOMWindowPerformance::performance(LocalDOMWindow& window) {
  return From(window).performance();
}

WindowPerformance* DOMWindowPerformance::performance() {
  if (!performance_)
    performance_ = WindowPerformance::Create(GetSupplementable());
  return performance_.Get();
}

}  // namespace blink
