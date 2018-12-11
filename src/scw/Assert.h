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

namespace scw {

////////////////////////////////////////////////////////////////////////////////
/// General purpose NOP (do nothing) function. Can help with warnings, static analysis
/// concerns, etc.
inline void nop() noexcept {}

////////////////////////////////////////////////////////////////////////////////
/// AssertAlways will be enabled even in release mode.
#ifdef SCW_MSVC_CXX
inline void AssertAlways(bool _cond) noexcept {
  if (!_cond)
    __debugbreak();
}
#else
inline void AssertAlways(bool _cond) noexcept {
  if (!_cond)
    std::abort();
}
#endif

// Override assert setting with -DSCW_ENABLE_ASSERT=0|1
#ifdef SCW_DEBUG
#  ifndef SCW_ENABLE_ASSERTS
#    define SCW_ENABLE_ASSERTS 1
#  endif
#endif

////////////////////////////////////////////////////////////////////////////////
/// Regular Assert. Aborts when enabled, otherwise does nothing.
#if SCW_ENABLE_ASSERTS
inline void Assert(bool _cond) noexcept {
  AssertAlways(_cond);
}
#else
inline void Assert(bool _cond) noexcept {
  nop();
}
#endif

}  // namespace scw
