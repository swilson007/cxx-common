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
#include <scw/Strings.h>

namespace scw {

/// Simple function to ensure our implicit convert to std::string
std::string toStr(const std::string& s) {
  return s;
}

TEST(StringsTest, testStringWrapperBasic) {
  StringWrapper s1 = "foobar";
  ASSERT_TRUE("foobar" == s1);
  ASSERT_TRUE(s1 == "foobar");

  std::string ss1 = "foobar";
  ASSERT_TRUE(ss1 == ss1);
  ASSERT_TRUE(ss1 == s1);
  ASSERT_TRUE(ss1 == s1.c_str());
}

}  // namespace scw
