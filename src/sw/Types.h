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

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <type_traits>

namespace sw {

// Arch sized types. No more int/uint. The 'x' suffix indicates "architecture"
// and is inspired by "x86" and "x64".
#if SW_ARCH_64BIT
using uintx = uint64_t;  // Arch sized unsigned int
using intx = int64_t;    // Arch sized int
#elif #if SW_ARCH_32BIT
using uintx = uint32_t;  // Arch sized unsigned int
using intx = int32_t;    // Arch sized int
#else
#  error "Unknown architecture"
#endif

// Alternate names for consistency
using sizex = std::size_t;
using ssizex = std::make_signed<size_t>::type;  // Arch sized int
using uptrx = uintptr_t;                        // Consistent name for uintptr_t
using ptrx = intptr_t;                          // Consistent name
using ptrdiffx = ptrdiff_t;                     // Consistent name

static_assert(sizeof(uintx) == SW_SIZEOF_POINTER, "Incorrect size");
static_assert(sizeof(intx) == sizeof(uintx), "Incorrect size");

// Tighter type names
using byte = std::uint8_t;
using i8 = std::int8_t;
using u8 = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

// convenience std:: typedefs
using MutexLock = std::lock_guard<std::mutex>;
using MutexUniqueLock = std::unique_lock<std::mutex>;
using SteadyClock = std::chrono::steady_clock;
using HiResClock = std::chrono::high_resolution_clock;
using HiResTimepoint = HiResClock::time_point;
using HiResTimepointNs = std::chrono::time_point<HiResClock, std::chrono::nanoseconds>;
using HiResTimepointMs = std::chrono::time_point<HiResClock, std::chrono::milliseconds>;
using SteadyTimepointMs = std::chrono::time_point<SteadyClock, std::chrono::milliseconds>;

}  // namespace sw
