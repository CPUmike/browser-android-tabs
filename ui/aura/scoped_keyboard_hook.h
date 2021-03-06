// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SCOPED_KEYBOARD_HOOK_H_
#define UI_AURA_SCOPED_KEYBOARD_HOOK_H_

#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "ui/aura/aura_export.h"

namespace aura {

class WindowTreeHost;

// Destroying an instance of this class will clean up the KeyboardHook instance
// owned by WindowTreeHost and prevent future system key events from being
// captured.  If the KeyboardHook or WindowTreeHost instances were already
// destroyed, then destroying this instance is a noop.
class AURA_EXPORT ScopedKeyboardHook {
 public:
  explicit ScopedKeyboardHook(base::WeakPtr<WindowTreeHost> weak_ptr);
  ~ScopedKeyboardHook();

 private:
  THREAD_CHECKER(thread_checker_);

  base::WeakPtr<WindowTreeHost> window_tree_host_;

  DISALLOW_COPY_AND_ASSIGN(ScopedKeyboardHook);
};

}  // namespace aura

#endif  // UI_AURA_SCOPED_KEYBOARD_HOOK_H_
