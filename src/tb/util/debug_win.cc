/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Seger�s and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cstdio>

#include "tb/util/debug.h"
#include "tb/util/string.h"

#define NOMINMAX
#include <windows.h>

#ifdef TB_RUNTIME_DEBUG_INFO

void TBDebugOut(const char* format, ...) {
  va_list va;
  va_start(va, format);
  auto result = tb::util::format_string(format, va);
  va_end(va);
  OutputDebugStringA(result.c_str());
}

#endif  // TB_RUNTIME_DEBUG_INFO