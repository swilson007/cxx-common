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

#include "assert.h"
#include "defines.h"
#include "fixed_width_int_literals.h"
#include "lazy.h"
#include "misc.h"
#include "types.h"

#include <codecvt>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

SW_NAMESPACE_BEGIN

using namespace sw::intliterals;

#if SW_POSIX
using WCharConverter = std::codecvt_utf8<wchar_t>;
#elif SW_WINDOWS
using WCharConverter = std::codecvt_utf8_utf16<wchar_t>;
#else
#  error Determine UTF appropriate char type for this platform. See https://en.cppreference.com/w/cpp/locale/codecvt
#endif

////////////////////////////////////////////////////////////////////////////////
/// Convert platform neutral utf8 string to a platfrom dependent wstring.
/// utf-32 on posix, utf-16 on Windows.
inline std::wstring widen(const std::string& u8str) {
  std::wstring_convert<WCharConverter> converter;
  return converter.from_bytes(u8str);
}

////////////////////////////////////////////////////////////////////////////////
/// Convert from platform dependent wstring (utf-32 on posix, utf-16 on Windows)
/// to platform neutral utf-8 string.
inline std::string narrow(const std::wstring& wstr) {
  std::wstring_convert<WCharConverter> converter;
  return converter.to_bytes(wstr);
}

////////////////////////////////////////////////////////////////////////////////
/// From cppreference.com:
///   Like all other functions from <cctype>, the behavior of std::isalpha is undefined if the
///   argument's value is neither representable as unsigned char nor equal to EOF
inline bool isalpha(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch));
}

////////////////////////////////////////////////////////////////////////////////
/// From cppreference.com:
///   Like all other functions from <cctype>, the behavior of std::isalpha is undefined if the
///   argument's value is neither representable as unsigned char nor equal to EOF
inline bool isalnum(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch));
}

////////////////////////////////////////////////////////////////////////////////
/// If the final character of the given string matches the provided 'trimChar',
/// that character will be removed and the string will be shorter by one character.
/// Otherwise, the string will be the same.
inline void trimEndingChar(std::string& str, char trimChar) {
  auto const len = str.length();
  if (len > 0 && str[len - 1] == trimChar) {
    str.resize(len - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Determines if 'str' ends with 'suffix'. This is for pre-c++ 20
template <typename StringClass>
inline bool endsWith(const StringClass& str, const std::string& suffix) {
  auto const strLen = str.length();
  auto const suffixLen = suffix.length();
  bool result =
      (strLen >= suffixLen) && (std::memcmp(&str[strLen - suffixLen], suffix.data(), suffixLen) == 0);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Determines if 'str' ends with the given char
inline bool endsWith(const std::string& str, char ch) {
  return !str.empty() && str.back() == ch;
}

////////////////////////////////////////////////////////////////////////////////
/// Determines if 'str' starts with 'prefix'. This is for pre-c++ 20
template <typename StringClass>
inline bool startsWith(const StringClass& str, const std::string& prefix) {
  auto const strLen = str.length();
  auto const prefixLen = prefix.length();
  bool result = (strLen >= prefixLen) && (std::memcmp(str.data(), prefix.data(), prefixLen) == 0);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Determines if 'str' ends with the given char
inline bool startsWith(const std::string& str, char ch) {
  return !str.empty() && str.front() == ch;
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
  static constexpr sizex kMaxStringLen = ~0_z >> 1;

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
  LazyPodValue<sizex, ~0_z> size_;
};

////////////////////////////////////////////////////////////////////////////////
/// Non-owning wrapper for std::string or const char* that is similar to std::string_view.
/// Just doing this for pre-C++17 code - keeping API as similar as possible.
///
/// Underlying string data is not owned by this class, and may not be null-terminated.
///
/// NOTE: Only partially implemented, add stuff as needed
////////////////////////////////////////////////////////////////////////////////
class StringView {
  static constexpr size_t kUnsetSize = ~size_t(0);
  static constexpr sizex kMaxStringLen = ~0_z >> 1u;

public:
  // Similar typedefs to std::string
  using const_iterator = char const*;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using const_reference = char const&;

  constexpr StringView() noexcept = default;
  ~StringView() noexcept = default;

  StringView(StringView const&) = default;
  StringView& operator=(StringView const&) = default;

  /// Wrap a const char* based string. The size will be computed immediatley
  /// Non-explicit intentionally
  StringView(char const* str) noexcept : _data(str), _size(std::strlen(str)) { SW_ASSERT(str != nullptr); }

  /// Wrap a const char* based string with a known size
  constexpr StringView(char const* str, size_t len) noexcept : _data(str), _size(len) {
    SW_ASSERT(str != nullptr);
    SW_ASSERT(len <= kMaxStringLen);
  }

  const_iterator begin() const { return &_data[0]; }
  const_iterator end() const { return &_data[size()]; }
  const_iterator cbegin() const { return &_data[0]; }
  const_iterator cend() const { return &_data[size()]; }
  const_reference operator[](size_t i) const { return _data[i]; }
  const_reference at(size_t i) const { return _data[i]; }
  const_reference front() const { return _data[0]; }
  const_reference back() const { return _data[size() - 1]; }

  const char* data() const { return _data; }
  size_t size() const { return _size; }
  size_t length() const { return _size; }
  bool empty() const { return _size == 0; }

  // clang-format off
  // Compares to const char* and std::string
  friend bool operator==(StringView const& lhs, StringView const& rhs) noexcept { return isEqual(lhs._data, lhs._size, rhs._data, rhs._size); }
  friend bool operator!=(StringView const& lhs, StringView const& rhs) noexcept { return !isEqual(lhs._data, lhs._size, rhs._data, rhs._size); }
  friend bool operator==(StringView const& lhs, char const* rhs) noexcept { return isEqual(lhs._data, lhs._size, rhs, strlen(rhs)); }
  friend bool operator!=(StringView const& lhs, char const* rhs) noexcept { return !isEqual(lhs._data, lhs._size, rhs, strlen(rhs)); }
  friend bool operator==(StringView const& lhs, std::string const& rhs) noexcept { return isEqual(lhs._data, lhs._size, rhs.c_str(), rhs.size()); }
  friend bool operator!=(StringView const& lhs, std::string const& rhs) noexcept { return !isEqual(lhs._data, lhs._size, rhs.c_str(), rhs.size()); }
  friend bool operator==(char const* lhs, StringView const& rhs) noexcept { return isEqual(lhs, strlen(lhs), rhs._data, rhs._size); }
  friend bool operator!=(char const* lhs, StringView const& rhs) noexcept { return !isEqual(lhs, strlen(lhs), rhs._data, rhs._size); }
  friend bool operator==(std::string const& lhs, StringView const& rhs) noexcept { return isEqual(lhs.c_str(), lhs.size(), rhs._data, rhs._size); }
  friend bool operator!=(std::string const& lhs, StringView const& rhs) noexcept { return !isEqual(lhs.c_str(), lhs.size(), rhs._data, rhs._size); }
  // clang-format on

  friend std::ostream& operator<<(std::ostream& outs, StringView const& sw) {
    return outs.write(sw.data(), static_cast<std::streamsize>(sw.size()));
  }

private:
  /// Comare if two non-null terminated string are equal
  static bool isEqual(const char* s1, sizex s1s, const char* s2, sizex s2s) {
    bool result = (s1s == s2s) ? std::memcmp(s1, s2, s1s) == 0 : false;
    return result;
  }

private:
  char const* _data = "";
  sizex _size = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Create a string from a string view
inline std::string toString(const StringView& sv) {
  return std::string(sv.data(), sv.size());
}

SW_NAMESPACE_END
