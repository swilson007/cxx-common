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
#include <sw/strings.h>

#include <gtest/gtest.h>

SW_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
TEST(StringsTest, testStringWrapperBasic) {
  {
    StringWrapper s1 = "foobar";
    ASSERT_TRUE("foobar" == s1);
    ASSERT_TRUE(s1 == "foobar");

    std::string ss1 = "foobar";
    ASSERT_TRUE(ss1 == s1);
    ASSERT_FALSE(ss1 != s1);
    ASSERT_FALSE(s1 != ss1);
    ASSERT_TRUE(ss1 == s1.c_str());
    ASSERT_FALSE(ss1 != s1.c_str());
  }

  {
    StringWrapper s1 = StringWrapper("foobar", 6);
    ASSERT_TRUE("foobar" == s1);
    ASSERT_TRUE(s1 == "foobar");
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST(StringsTest, testStringViewBasic) {
  {
    StringView s1 = "foobar";
    ASSERT_TRUE("foobar" == s1);
    ASSERT_TRUE("xfoobar" != s1);
    ASSERT_TRUE(s1 == "foobar");
    ASSERT_TRUE(s1 != "xfoobar");

    std::string ss1 = "foobar";
    ASSERT_TRUE(ss1 == s1);
    ASSERT_FALSE(ss1 != s1);
    ASSERT_FALSE(s1 != ss1);
  }

  {
    StringView s1 = StringView("foobar", 6);
    ASSERT_TRUE("foobar" == s1);
    ASSERT_TRUE("xfoobar" != s1);
    ASSERT_TRUE(s1 == "foobar");
    ASSERT_TRUE(s1 != "xfoobar");
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST(StringsTest, endsWith) {
  std::string s = "foobar.exe";
  ASSERT_TRUE(sw::endsWith(s, ".exe"));
  ASSERT_TRUE(sw::endsWith(s, "exe"));
  ASSERT_TRUE(sw::endsWith(s, "xe"));
  ASSERT_FALSE(sw::endsWith(s, ".ex"));
  ASSERT_FALSE(sw::endsWith(s, "foobar.exe2"));

  StringView sv = "foobar.exe";
  ASSERT_TRUE(sw::endsWith(sv, ".exe"));
  ASSERT_TRUE(sw::endsWith(sv, "exe"));
  ASSERT_TRUE(sw::endsWith(sv, "xe"));
  ASSERT_FALSE(sw::endsWith(sv, ".ex"));
  ASSERT_FALSE(sw::endsWith(sv, "foobar.exe2"));
}

////////////////////////////////////////////////////////////////////////////////
TEST(StringsTest, startsWith) {
  std::string s = "foobar.exe";
  ASSERT_TRUE(sw::startsWith(s, "foobar"));
  ASSERT_TRUE(sw::startsWith(s, "foo"));
  ASSERT_TRUE(sw::startsWith(s, "f"));
  ASSERT_FALSE(sw::startsWith(s, "oobar"));
  ASSERT_FALSE(sw::startsWith(s, "foobar.exef"));

  StringView sv = "foobar.exe";
  ASSERT_TRUE(sw::startsWith(sv, "foobar"));
  ASSERT_TRUE(sw::startsWith(sv, "foo"));
  ASSERT_TRUE(sw::startsWith(sv, "f"));
  ASSERT_FALSE(sw::startsWith(sv, "oobar"));
  ASSERT_FALSE(sw::startsWith(sv, "foobar.exef"));
}

SW_NAMESPACE_END
