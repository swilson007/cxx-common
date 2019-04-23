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

#include "fixed_width_int_literals.h"
#include "types.h"
#include "utils.h"

#include <string>

namespace sw {

/// Base64 encoder that supports:
///  * Standard encoding, or
///  * URL/Filename safe encoding per https://en.wikipedia.org/wiki/Base64#Variants_summary_table
///  * Optional '=' padding. URL/Filename version defaults to no padding, Standard defaults to padded

namespace detail {

struct Base64Traits {
  static constexpr const char* kEncode =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  static char encode(byte bits6) {
    SW_ASSERT((bits6 & ~0b111111_u8) == 0);
    return kEncode[bits6];
  }
};

struct Base64UrlTraits {
  static constexpr const char* kEncode =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  static char encode(byte bits6) {
    SW_ASSERT((bits6 & ~0b111111_u8) == 0);
    return kEncode[bits6];
  }
};

}  // namespace detail

////////////////////////////////////////////////////////////////////////////////
/// The main encoding function, defined later
template <typename Base64Traits = ::sw::detail::Base64Traits>
inline std::string base64Encoder(const byte source[], sizex sourceLen, bool pad);

////////////////////////////////////////////////////////////////////////////////
/// Do standard base64 encoding
inline std::string base64Encode(const byte source[], sizex sourceLen, bool pad = true) {
  return base64Encoder(source, sourceLen, pad);
}

////////////////////////////////////////////////////////////////////////////////
inline std::string base64Encode(const std::string& str, bool pad = true) {
  return base64Encode(sw::utils::saferAlias<const byte*>(str.data()), str.size(), pad);
}

////////////////////////////////////////////////////////////////////////////////
inline std::string base64Encode(const char* str, sizex len, bool pad = true) {
  return base64Encode(sw::utils::saferAlias<const byte*>(str), len, pad);
}

////////////////////////////////////////////////////////////////////////////////
/// Do url & filename safe base64 encoding
inline std::string base64UrlEncode(const byte source[], sizex sourceLen, bool pad = false) {
  return base64Encoder<::sw::detail::Base64UrlTraits>(source, sourceLen, pad);
}

////////////////////////////////////////////////////////////////////////////////
inline std::string base64UrlEncode(const std::string& str, bool pad = false) {
  return base64UrlEncode(sw::utils::saferAlias<const byte*>(str.data()), str.size(), pad);
}

////////////////////////////////////////////////////////////////////////////////
inline std::string base64UrlEncode(const char* str, sizex len, bool pad = false) {
  return base64UrlEncode(sw::utils::saferAlias<const byte*>(str), len, pad);
}

////////////////////////////////////////////////////////////////////////////////
template <typename Base64Traits>
inline std::string base64Encoder(const byte source[], sizex sourceLen, bool pad) {
  SW_ASSERT(((sourceLen << 2u) >> 2u) ==
            sourceLen);  // Ensure 2-MSbits are zero so we don't overflow

  // Figure out resultant string size. It will be much more efficient to pre-size the
  // string and use [] ops than to append to it, as the append func has to do various checks
  // like figuring out if it needs to regrow the string. Also, we'll only incur one
  // allocation this way
  const sizex baseMod = sourceLen % 3;
  const sizex baseExtra = baseMod > 0 ? 3 - baseMod : 0;
  const sizex baseLen = sourceLen + baseExtra;
  const sizex resultSize = (baseLen / 3 * 4) - (pad ? 0 : baseExtra);
  auto result = std::string(resultSize, '\0');
  sizex resultPos = 0;

  const auto encode = [](byte bits6) -> char { return Base64Traits::encode(bits6); };

  // Work through the array in 3-byte groups, encode every 6-bits into a single char
  const byte* pos = source;
  const byte* const end = pos + sourceLen;
  while ((pos + 2) < end) {
    // Encode 3-bytes
    const auto& b0 = *pos++;
    const auto& b1 = *pos++;
    const auto& b2 = *pos++;

    // Note - shift operators promote byte's to ints, thus our cast-fest
    result[resultPos++] = encode(byte(b0 >> 2u));
    result[resultPos++] = encode(byte(byte(byte(b0 << 4u) & 0b110000_u8) | byte(b1 >> 4u)));
    result[resultPos++] = encode(byte(byte(byte(b1 << 2u) & 0b111100_u8) | byte(b2 >> 6u)));
    result[resultPos++] = encode(byte(b2 & 0b111111_u8));
  }

  // Groups of 3 complete. We're left with 0, 1, or 2 more bytes to encode.
  constexpr char kPad = '=';
  if ((pos + 2) == end) {  // 2 bytes left
    const auto& b0 = *pos++;
    const auto& b1 = *pos++;
    result[resultPos++] = encode(byte(b0 >> 2u));
    result[resultPos++] = encode(byte(byte(byte(b0 << 4u) & 0b110000_u8) | byte(b1 >> 4u)));
    result[resultPos++] = encode(byte(byte(b1 << 2u) & 0b111100_u8));
    if (pad) {
      result[resultPos++] = kPad;
    }
  } else if ((pos + 1) == end) { // 1 byte left
    const auto& b0 = *pos++;
    result[resultPos++] = encode(byte(b0 >> 2u));
    result[resultPos++] = encode(byte(byte(b0 << 4u) & 0b110000_u8));
    if (pad) {
      result[resultPos++] = kPad;
      result[resultPos++] = kPad;
    }
  }

  return result;
}

/// TODO: write a decoder

}  // namespace sw
