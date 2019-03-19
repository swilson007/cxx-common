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

#include "Assert.h"
#include "FixedWidthIntLiterals.h"
#include "Misc.h"
#include "Types.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

namespace sw {

using namespace sw::intliterals;

////////////////////////////////////////////////////////////////////////////////
/// If the final character of the given string matches the provided 'trimChar',
/// that character will be removed and the string will be shorter by one character.
/// Otherwise, the string will be the same.
inline void trimEndingChar(std::string& str, char trimChar) {
  auto const len = str.length();
  if (len > 0 && str[len-1] == trimChar) {
    str.resize(len - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Non-owning wrapper for std::string or const char*. This is intended to solve
/// the conundrum of needing a function that takes a const char* vs a const
/// std::string&, and always avoiding a copy. Unlike std::string_view, StringWrapper
/// strings are always null-terminated which is key for their use case.
///
/// Wrapped string is undefined if given a nullptr for const char* case. For an empty
/// string, the size of the underlying buffer must be at least 1 and contain a '\0'
/// as the first character.
///
/// No allocations are performed when creating any instances.  The size will be computed
/// lazily for the case where a const char* was given.
////////////////////////////////////////////////////////////////////////////////
class StringWrapper {
  static constexpr sizex kUnsetSize = ~0_z;
#if SW_ARCH_64BIT
  static constexpr sizex kMaxStringLen = 0_z << 63;
#else
  static constexpr sizex kMaxStringLen = 0_z << 31;
#endif

public:
  // Similar typedefs to std::string
  using const_iterator = const char*;
  using size_type = sizex;
  using difference_type = ptrdiffx;
  using const_reference = const char&;

  StringWrapper() noexcept = default;
  ~StringWrapper() noexcept = default;

  StringWrapper(const StringWrapper&) = default;
  StringWrapper& operator=(const StringWrapper&) = default;
  StringWrapper(StringWrapper&&) noexcept = default;
  StringWrapper& operator=(StringWrapper&&) noexcept = default;

  /// Wrap a const char* based string. The size will be computed lazily when needed.
  /// Non-explicit intentionally
  StringWrapper(const char* str) noexcept : data_(str) { SW_ASSERT(str != nullptr); }

  /// Wrap a std::string
  /// Non-explicit intentionally
  StringWrapper(const std::string& str) noexcept : data_(str.data()), size_(str.size()) {}

  /// Wrap a const char* based string with a known size
  StringWrapper(const char* str, sizex len) noexcept : data_(str), size_(len) {
    SW_ASSERT(len <= kMaxStringLen);
    SW_ASSERT(data_[len] == 0);  // Correct length?
  }

  const_iterator begin() const { return &data_[0]; }
  const_iterator end() const { return &data_[size()]; }
  const_iterator cbegin() const { return &data_[0]; }
  const_iterator cend() const { return &data_[size()]; }
  const_reference operator[](sizex i) const { return data_[i]; }
  const_reference at(sizex i) const { return data_[i]; }
  const_reference front() const { return data_[0]; }
  const_reference back() const { return data_[size() - 1]; }

  const char* data() const { return data_; }
  const char* c_str() const { return data_; }

  sizex size() const {
    return size_.get([&]() { return std::strlen(data_); });
  }

  sizex length() const { return size(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// A string will be deemed empty if 1) the first char is '\0', or 2) if the size is zero. We
  /// include (1) so that we can avoid checking the size if possible (since the size is lazy)
  bool empty() const { return data()[0] == '\0' || size() == 0; }

  // Compares to const char* and std::string
  // @formatter:off
  // clang-format off
  friend bool operator==(const StringWrapper& lhs, const StringWrapper& rhs) noexcept { return std::strcmp(lhs.data_, rhs.data_) == 0; }
  friend bool operator==(const StringWrapper& lhs, const char* rhs) noexcept { return std::strcmp(lhs.data_, rhs) == 0; }
  friend bool operator!=(const StringWrapper& lhs, const char* rhs) noexcept { return std::strcmp(lhs.data_, rhs) != 0; }
  friend bool operator==(const StringWrapper& lhs, const std::string& rhs) noexcept { return std::strcmp(lhs.data_, rhs.c_str()) == 0; }
  friend bool operator!=(const StringWrapper& lhs, const std::string& rhs) noexcept { return std::strcmp(lhs.data_, rhs.c_str()) != 0; }
  friend bool operator==(const char* lhs, const StringWrapper& rhs) noexcept { return std::strcmp(lhs, rhs.data_) == 0; }
  friend bool operator!=(const char* lhs, const StringWrapper& rhs) noexcept { return std::strcmp(lhs, rhs.data_) != 0; }
  friend bool operator==(const std::string& lhs, const StringWrapper& rhs) noexcept { return std::strcmp(lhs.c_str(), rhs.data_) == 0; }
  friend bool operator!=(const std::string& lhs, const StringWrapper& rhs) noexcept { return std::strcmp(lhs.c_str(), rhs.data_) != 0; }

  friend std::ostream& operator<<(std::ostream& outs, const StringWrapper& sw) { return outs << sw.c_str(); }
  // clang-format on
  // @formatter:on

private:
  const char* data_ = nullptr;

  // Size is lazy because typical usage might be to take a const char* and just pass it to a
  // function, never using the size of the string.
  LazyValue<sizex, ~0_z> size_;
};

}  // namespace sw
