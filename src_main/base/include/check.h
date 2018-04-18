// Copyright © 1996-2018, Valve Corporation, All rights reserved.

#ifndef BASE_INCLUDE_CHECK_H_
#define BASE_INCLUDE_CHECK_H_

#include <cassert>
#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
// Just use |code_|.
#define SOURCE_DBG_CODE_NOSCOPE(code_) code_
#else  // NDEBUG
// Skip |code_|.
#define SOURCE_DBG_CODE_NOSCOPE(code_) ((void)0)
#endif  // !NDEBUG

// Checks |condition| in DEBUG mode, if not, then print condition and
// |exit_code| to stderr and exit process with |exit_code|. Process is
// terminated via std::exit call.
#define DCHECK(condition, exit_code)                   \
  assert(condition);                                   \
  SOURCE_DBG_CODE_NOSCOPE(if (!(condition)) {          \
    fprintf_s(stderr, "%s failed (%d).", (#condition), \
              static_cast<int>(exit_code));            \
    std::exit(exit_code);                              \
  })

// Checks |condition|, if not, then print condition and |exit_code| to stderr
// and exit process with |exit_code|. Process is terminated via std::exit call.
#define CHECK(condition, exit_code)                    \
  assert(condition);                                   \
  if (!(condition)) {                                  \
    fprintf_s(stderr, "%s failed (%d).", (#condition), \
              static_cast<int>(exit_code));            \
    std::exit(exit_code);                              \
  }

#endif  // BASE_INCLUDE_CHECK_H_