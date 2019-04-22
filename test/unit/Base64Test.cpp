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
#include <sw/base64.h>
#include <sw/uuid.h>

#include <gtest/gtest.h>

#include <iostream>

namespace sw {

TEST(Base64Test, basic) {
  std::cout << "b64('Mang')='" << base64Encode("Mang") << "'" << std::endl;
  std::cout << "b64('Man')='" << base64Encode("Man") << "'" << std::endl;
  std::cout << "b64('Ma')='" << base64Encode(std::string("Ma")) << "'" << std::endl;
  std::cout << "b64('M')='" << base64Encode(std::string("M")) << "'" << std::endl;

  std::cout << "b64('Mang')='" << base64FilenameEncode("Mang") << "'" << std::endl;
  std::cout << "b64('Man')='" << base64FilenameEncode("Man") << "'" << std::endl;
  std::cout << "b64('Ma')='" << base64FilenameEncode(std::string("Ma")) << "'" << std::endl;
  std::cout << "b64('M')='" << base64FilenameEncode(std::string("M")) << "'" << std::endl;

  ASSERT_EQ(base64Encode("Encode to Base64 format"), "RW5jb2RlIHRvIEJhc2U2NCBmb3JtYXQ=");
  ASSERT_EQ(base64Encode("Easy to"), "RWFzeSB0bw==");
  ASSERT_EQ(base64Encode("<>?"), "PD4/");

  ASSERT_EQ(base64FilenameEncode("<>?"), "PD4_");
}

TEST(Base64Test, uuid) {
  for (sizex i = 0; i < 10; ++i) {
    Uuid u1 = Uuid::create();
    std::cout << "u1=" << u1.toString() << ", b64=" << u1.toBase64()
              << ", b64f=" << u1.toBase64Filename() << std::endl;
  }
}

}  // namespace sw
