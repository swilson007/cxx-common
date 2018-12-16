#pragma once

#include "System.h"
#include "Utils.h"

#include <array>
#include <iostream>

namespace scw {

template <typename System = system::ThisSystem>
class UuidType;
using Uuid = UuidType<>;

////////////////////////////////////////////////////////////////////////////////
/// Class that represents a UUID
////////////////////////////////////////////////////////////////////////////////
template <typename System>
class UuidType {
public:
  /// Creates a random UUID
  static UuidType create() noexcept;

  /// Creates an invalid UUID
  UuidType() noexcept = default;

  /// Creates a UUID from the given bytes.
  explicit UuidType(const std::array<byte, 16>& bytes) noexcept : bytes_(bytes) {}

  /// Creates a UUID from the 16 bytes starting at the given byte pointer
  explicit UuidType(const byte* bytes) noexcept { ::memcpy(bytes_.data(), bytes, bytes_.size()); }

  // Copy/Move is standard
  UuidType(const UuidType& other) noexcept = default;
  UuidType& operator=(const UuidType& other) noexcept = default;
  UuidType(UuidType&& other) noexcept = default;
  UuidType& operator=(UuidType&& other) noexcept = default;

  // TODO Uuid(const StringWrapper& string);

  ////////////////////////////////////////////////////////////////////////////////
  /// Convert the UUID into a string
  std::string toString() const {
    std::array<char, 36> buffer = {{0}};
    for (size_t i = 0, pos = 0; i < 16; ++i) {
      if (i == 4 || i == 6 || i == 8 || i == 10) {
        buffer[pos++] = '-';
      }
      auto hexChar = utils::toHexChar(bytes_[i]);
      buffer[pos++] = hexChar[0];
      buffer[pos++] = hexChar[1];
    }

    std::string result(buffer.data(), 36);
    return result;
  }

  /// Determine if the UUID is valid or not. An invalid UUID has all bytes as 0
  inline bool isValid() const {
    auto invalid = Uuid();
    return *this != invalid;
  }

  friend bool operator==(const UuidType& a, const UuidType& b) { return a.bytes_ == b.bytes_; }
  friend bool operator!=(const UuidType& a, const UuidType& b) { return a.bytes_ != b.bytes_; }
  friend bool operator<(const UuidType& a, const UuidType& b) { return a.bytes_ < b.bytes_; }
  friend bool operator<=(const UuidType& a, const UuidType& b) { return a.bytes_ <= b.bytes_; }
  friend bool operator>(const UuidType& a, const UuidType& b) { return a.bytes_ > b.bytes_; }
  friend bool operator>=(const UuidType& a, const UuidType& b) { return a.bytes_ >= b.bytes_; }
  friend std::ostream& operator<<(std::ostream& outs, const UuidType& u) {
    return outs << u.toString().c_str();
  }

private:
  std::array<byte, 16> bytes_ = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
};

}  // namespace scw

////////////////////////////////////////////////////////////////////////////////
/// Per-OS implementation.
///
/// SCW: I really don't like including OS headers here, but no other way to do this
///  for "header-only".
////////////////////////////////////////////////////////////////////////////////

#if SCW_UNIX

#include <uuid/uuid.h>
namespace scw {

////////////////////////////////////////////////////////////////////////////////
template <>
Uuid Uuid::create() noexcept {
  // NOTE: uuid_t is a uint8[16], so we can pass it directly to our constructor
  uuid_t id;
  ::uuid_generate(id);
  return Uuid(id);
}

#elif SCW_WINDOWS
# error Implement Uuid::create for windows
#endif

}  // namespace scw
