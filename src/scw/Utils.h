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
#include "Strings.h"
#include "System.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace scw { namespace utils {

template <typename System = ::scw::system::ThisSystem>
struct UtilsStorageType;
using UtilsStorage = UtilsStorageType<>;

////////////////////////////////////////////////////////////////////////////////
template <typename System>
struct UtilsStorageType {
  // Maximum supported format string length
  static const std::string kErrorString;

  ////////////////////////////////////////////////////////////////////////////////
  static constexpr const std::array<const char*, 256> kHexLookup = {
      {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e",
       "0f", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d",
       "1e", "1f", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c",
       "2d", "2e", "2f", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b",
       "3c", "3d", "3e", "3f", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a",
       "4b", "4c", "4d", "4e", "4f", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
       "5a", "5b", "5c", "5d", "5e", "5f", "60", "61", "62", "63", "64", "65", "66", "67", "68",
       "69", "6a", "6b", "6c", "6d", "6e", "6f", "70", "71", "72", "73", "74", "75", "76", "77",
       "78", "79", "7a", "7b", "7c", "7d", "7e", "7f", "80", "81", "82", "83", "84", "85", "86",
       "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f", "90", "91", "92", "93", "94", "95",
       "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f", "a0", "a1", "a2", "a3", "a4",
       "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af", "b0", "b1", "b2", "b3",
       "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf", "c0", "c1", "c2",
       "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf", "d0", "d1",
       "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df", "e0",
       "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
       "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe",
       "ff"}};
};

////////////////////////////////////////////////////////////////////////////////
template <typename System>
const std::string UtilsStorageType<System>::kErrorString = "<error>";

template <typename System>
const std::array<const char*, 256> UtilsStorageType<System>::kHexLookup;

////////////////////////////////////////////////////////////////////////////////
/// Convert a byte to it's hex string. Quickly.
inline constexpr char const* toHexChar(byte b) {
  return UtilsStorage::kHexLookup[b];
}

////////////////////////////////////////////////////////////////////////////////
/// sprintf style string formatter that places results into the provided buffer. sprintf
/// will be called at most once.
///
/// TODO: Reevaluate this and move to {fmt}
///
/// @param output Buffer to output to
/// @param output Length in bytes of the buffer
/// @return result of underlying call to snprintf
template <typename... Ts>
int formatInto(char* output, sizex outputSize, const StringWrapper& format, Ts... ts) {
  Assert(outputSize > 0);

  // Special case of empty format string
  if (format.empty()) {
    output[0] = 0;
    return 0;
  }

  // Ensure reasonable size
  outputSize = std::min(outputSize, static_cast<sizex>(std::numeric_limits<int>::max()));

  // Format into the output buffer. Note that formatSize can be larger than maxLength, but only
  // maxLength bytes will have been written
  auto formatSize = std::snprintf(output, outputSize, format.c_str(), std::forward<Ts>(ts)...);
  if (formatSize < 0) {
    Assert(true);
    output[0] = 0;
  }

  return formatSize;
}

////////////////////////////////////////////////////////////////////////////////
/// sprintf style string formatter capped at maxLength.  This will call std::snprintf once.
///
/// TODO: Reevaluate this and move to {fmt}
///
///
/// @param maxLength Maximum length of the resultant string, NOT including the null terminator
///                Thus the resultant string will be "result.length() <= maxSize"
/// @return Formatted string, or "<error>" if an error occured
template <sizex bufferSize = 1024, typename... Ts>
std::string formatn(const StringWrapper& formatStr, Ts... ts) {
  std::array<char, bufferSize> buffer;
  int size = formatInto(buffer.data(), bufferSize, formatStr, std::forward<Ts>(ts)...);
  if (size < 0) {
    return UtilsStorage::kErrorString;
  } else {
    sizex finalSize = std::min(static_cast<sizex>(size), bufferSize - 1);
    auto result = std::string(buffer.data(), finalSize);
    return result;
  };
}

////////////////////////////////////////////////////////////////////////////////
/// Alternative to reinterpret-cast for aliasing. Source and Dest must have the
/// same sizeof(). Note this is "safer" and not "safe" because the results may
/// still be aliased types, and may violate the Type Aliasing rules. But, the memcpy
/// approach is supposed to be an acceptable technique to inform the compiler
/// of what we're doing vs using reinterpret_cast.
///
/// Note that compilers are trained to see through this and produce the same code as
/// a reinterpret_cast expression.
template <typename Dest, typename Source>
inline void saferAlias(Dest& dest, Source&& source) {
  static_assert(sizeof(Dest) == sizeof(Source), "Sizes must be the same.");
  std::memcpy(&dest, &source, sizeof(Dest));
}

////////////////////////////////////////////////////////////////////////////////
/// Alternative to reinterpret-cast for extracting type-punned data from a buffer.
/// Note that compilers are trained to see through this and produce the same code as
/// a reinterpret_cast expression
template <typename Dest>
inline Dest extractFromBuffer(const byte* sourceBuffer) {
  Dest dest;
  std::memcpy(&dest, sourceBuffer, sizeof(Dest));
  return dest;
}

////////////////////////////////////////////////////////////////////////////////
/// Alternative to reinterpret-cast for placing type-punned data into a buffer.
/// Note that compilers are trained to see through this and produce the same code as
/// a reinterpret_cast expression
template <typename Source>
inline void placeIntoBuffer(byte* destBuffer, Source value) {
  std::memcpy(destBuffer, &value, sizeof(Source));
}

}}  // namespace scw::utils
