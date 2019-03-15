////////////////////////////////////////////////////////////////////////////////
/// Copyright 2018 Steven C. Wilson
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
/// and associated documentation files (the "Software"), to deal in the Software without
/// restriction, including without limitation the rights to use, copy, modify, merge, publish,
/// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
/// Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or
/// substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
/// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
/// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Defines.h"

#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////
/// Non-macro asserts.
////////////////////////////////////////////////////////////////////////////////

namespace sw {

////////////////////////////////////////////////////////////////////////////////
/// General purpose NOP (do nothing) function. Can help with warnings, static analysis
/// concerns, etc.
inline void nop() noexcept {}

////////////////////////////////////////////////////////////////////////////////
/// AssertAlways will be enabled even in release mode.
/// Using MACROs for the asserts so we can add __FILE__ and __LINE__ in when needed
#ifdef SW_MSVC_CXX
inline void AssertAlwaysImpl(bool cond) noexcept {
  if (!cond)
    __debugbreak();
}
#else
inline void AssertAlwaysImpl(bool cond) noexcept {
  if (!cond)
    std::abort();
}
#endif
#define SW_ASSERT_ALWAYS(cond_) AssertAlwaysImpl(cond_)

// Override assert setting with -DSW_ENABLE_ASSERT=0|1
#ifdef SW_DEBUG
#  ifndef SW_ENABLE_ASSERTS
#    define SW_ENABLE_ASSERTS 1
#  endif
#endif

////////////////////////////////////////////////////////////////////////////////
/// Regular Assert. Aborts when enabled, otherwise does nothing.
#if SW_ENABLE_ASSERTS
inline void AssertImpl(bool cond) noexcept {
  AssertAlwaysImpl(cond);
}
#define SW_ASSERT(cond_) AssertImpl(cond_)
#else
#define SW_ASSERT(cond_)
#endif

}  // namespace sw
