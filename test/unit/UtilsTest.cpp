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
#include <gtest/gtest.h>
#include <scw/Utils.h>

#include <array>

namespace scw {

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testFormatInto) {
  constexpr sizex kBufSize = 20;
  std::array<char, kBufSize> buf = {{0}};
  auto formatLen = utils::formatInto(buf.data(), kBufSize, "Hello: %d!=%d", 1, 2);
  ASSERT_EQ(11, formatLen);
  ASSERT_STREQ("Hello: 1!=2", buf.data());
}

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testFormatn) {
  auto const& str = utils::formatn("Hello: %d!=%d", 1, 2);
  ASSERT_EQ(11, str.length());
  ASSERT_STREQ("Hello: 1!=2", str.c_str());
}

}  // namespace scw
