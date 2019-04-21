////////////////////////////////////////////////////////////////////////////////
/// Copyright 2019 Steven C. Wilson
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

namespace sw {

////////////////////////////////////////////////////////////////////////////////
/// General purpose NOP (do nothing) function. Can help with warnings, static analysis
/// concerns, etc.
inline void nop() noexcept {}

////////////////////////////////////////////////////////////////////////////////
/// Tag a variable as unused
template <typename T>
inline void unused(const T&) noexcept {}

////////////////////////////////////////////////////////////////////////////////
/// SW_ASSERT_ALWAYS will be enabled even in release mode.
/// TODO: add __FILE__ and __LINE__ in once it's useful
#if SW_MSVC_CXX
#  define SW_ASSERT_ALWAYS(cond_) do {if (!(cond_)) __debugbreak();} while(0)
#elif defined(__has_builtin)
#  define SW_ASSERT_ALWAYS(cond_) do {if (!(cond_)) __builtin_trap();} while(0)
#else
#  define SW_ASSERT_ALWAYS(cond_) do {if (!(cond_)) std::abort();} while(0)
#endif

// Override assert setting with -DSW_ENABLE_ASSERT=0|1
#if defined(SW_DEBUG)
#  if !defined(SW_ENABLE_ASSERTS)
#    define SW_ENABLE_ASSERTS 1
#  endif
#endif

////////////////////////////////////////////////////////////////////////////////
/// SW_ASSERT is auto-enabled for debug builds while disabled for other builds.
/// Override it by setting SW_ENABLE_ASSERTS as a compile def.
#if SW_ENABLE_ASSERTS
#  define SW_ASSERT(cond_) SW_ASSERT_ALWAYS(cond_)
#else
#  define SW_ASSERT(cond_) (::sw::nop())
#endif

}  // namespace sw
