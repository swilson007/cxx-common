#pragma once

#include "Types.h"

#include <iostream>

#if SCW_POSIX
#  include <sys/types.h>
#  include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////
/// Contains compile time system information.
////////////////////////////////////////////////////////////////////////////////

namespace scw { namespace system {

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
template <SystemPosix Posix, SystemPlatform Platform, SystemArch Arch, sizex kArchPointerSize>
struct SystemInfo {
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

#if SCW_POSIX
constexpr SystemPosix kThisPosix = SystemPosix::Enabled;
#else
constexpr SystemPosix kThisPosix = SystemPosix::Disabled;
#endif

#if SCW_MACOS
constexpr SystemPlatform kThisPlatform = SystemPlatform::MacOs;
#elif SCW_LINUX
constexpr SystemPlatform kThisPlatform = SystemPlatform::Linux;
#elif SCW_WINDOWS
constexpr SystemPlatform kThisPlatform = SystemPlatform::Windows;
#else
#  error TODO
#endif

/// Setup each current system value
#if SCW_ARCH_32BIT
constexpr SystemArch kThisArch = SystemArch::Bits32;
#else
constexpr SystemArch kThisArch = SystemArch::Bits64;
#endif

/// Define the global system
using ThisSystem = SystemInfo<kThisPosix, kThisPlatform, kThisArch, SCW_SIZEOF_POINTER>;

}}  // namespace scw::system
