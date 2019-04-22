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

#include "types.h"

#include <iostream>

#if SW_POSIX
#  include <sys/types.h>
#  include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////
/// Contains compile time system information.
////////////////////////////////////////////////////////////////////////////////

namespace sw { namespace system {

////////////////////////////////////////////////////////////////////////////////
enum class SystemPosix : u8 {
  Disabled = 0,
  Enabled = 1,
};

////////////////////////////////////////////////////////////////////////////////
enum class SystemPlatform : u8 {
  Linux,
  MacOs,
  Windows,
};

////////////////////////////////////////////////////////////////////////////////
enum class SystemArch : u8 {
  Bits32,
  Bits64,
};

////////////////////////////////////////////////////////////////////////////////
inline constexpr char const* toString(SystemPosix v) {
  switch (v) {
  case SystemPosix::Enabled:
    return "Enabled";
  case SystemPosix::Disabled:
    return "Disabled";
  }
  return "unknown";
}

////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator<<(std::ostream& outs, SystemPosix v) {
  outs << toString(v);
  return outs;
}

////////////////////////////////////////////////////////////////////////////////
inline constexpr char const* toString(SystemArch v) {
  switch (v) {
  case SystemArch::Bits32:
    return "Bits32";
  case SystemArch::Bits64:
    return "Bits64";
  }
  return "unknown";
}

////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator<<(std::ostream& outs, SystemArch v) {
  outs << toString(v);
  return outs;
}

////////////////////////////////////////////////////////////////////////////////
inline constexpr char const* toString(SystemPlatform v) {
  switch (v) {
  case SystemPlatform::MacOs:
    return "MacOs";
  case SystemPlatform::Linux:
    return "Linux";
  case SystemPlatform::Windows:
    return "Windows";
  }
  return "unknown";
}

////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator<<(std::ostream& outs, SystemPlatform v) {
  outs << toString(v);
  return outs;
}

////////////////////////////////////////////////////////////////////////////////
template <SystemPosix Posix, SystemPlatform Platform, SystemArch Arch,
  sizex kArchPointerSize>
struct SystemTraits {
  static constexpr SystemPosix kPosix = Posix;
  static constexpr SystemPlatform kPlatform = Platform;
  static constexpr SystemArch kArch = Arch;
  static constexpr bool kIsPosix = (kPosix == SystemPosix::Enabled);
  static constexpr sizex kPointerSize = kArchPointerSize;

  // Function versions... for those times when you need a function
  static constexpr SystemPosix posix() { return kPosix; };
  static constexpr SystemPlatform platform() { return kPlatform; }
  static constexpr SystemArch arch() { return kArch; }
  static constexpr bool isPosix() { return kIsPosix; };
};

#if SW_POSIX
constexpr SystemPosix kThisPosix = SystemPosix::Enabled;
#else
constexpr SystemPosix kThisPosix = SystemPosix::Disabled;
#endif

/// Setup each current system value
#if SW_ARCH_32BIT
constexpr SystemArch kThisArch = SystemArch::Bits32;
#else
constexpr SystemArch kThisArch = SystemArch::Bits64;
#endif

// Finall - define the per-OS system traits class
#if SW_MACOS
constexpr SystemPlatform kThisPlatform = SystemPlatform::MacOs;
struct MacSystemTraits : public SystemTraits<kThisPosix, kThisPlatform, kThisArch, SW_SIZEOF_POINTER>
{
  static constexpr const char* kNewline = "\n";
  static constexpr const char* newline() { return kNewline; }
};

using ThisSystemTraits = MacSystemTraits;

#elif SW_LINUX
constexpr SystemPlatform kThisPlatform = SystemPlatform::Linux;
struct LinuxSystemTraits : public SystemTraits<kThisPosix, kThisPlatform, kThisArch, SW_SIZEOF_POINTER>
{
  static constexpr const char* kNewline = "\n";
  static constexpr const char* newline() { return kNewline; }
};

using ThisSystemTraits = LinuxSystemTraits;

#elif SW_WINDOWS
constexpr SystemPlatform kThisPlatform = SystemPlatform::Windows;
struct LinuxSystemTraits : public SystemTraits<kThisPosix, kThisPlatform, kThisArch, SW_SIZEOF_POINTER>
{
  static constexpr const char* kNewline = "\r\n";
  static constexpr const char* newline() { return kNewline; }
};

using ThisSystemTraits = WindowsSystemTraits;

#else
#  error TODO
#endif

}}  // namespace sw::system
