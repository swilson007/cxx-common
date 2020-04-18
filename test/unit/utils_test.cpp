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
#include <sw/utils.h>

#include <gtest/gtest.h>

#include <array>

SW_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testSplitString) {
  {
    auto subs = std::vector<std::string>{};
    utils::splitString("foo:bar:foobar", ':', [&](std::string&& s) { subs.emplace_back(std::move(s)); });
    ASSERT_EQ(subs.size(), 3u);
    ASSERT_EQ(subs[0], "foo");
    ASSERT_EQ(subs[1], "bar");
    ASSERT_EQ(subs[2], "foobar");
  }

  {
    auto subs = std::vector<std::string>{};
    utils::splitString(":foo::bar:foobar:", ':', [&](std::string&& s) { subs.emplace_back(std::move(s)); });
    ASSERT_EQ(subs.size(), 6u);
    ASSERT_EQ(subs[0], "");
    ASSERT_EQ(subs[1], "foo");
    ASSERT_EQ(subs[2], "");
    ASSERT_EQ(subs[3], "bar");
    ASSERT_EQ(subs[4], "foobar");
    ASSERT_EQ(subs[5], "");
  }
}

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
  ASSERT_EQ(11u, str.length());
  ASSERT_STREQ("Hello: 1!=2", str.c_str());
}

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testFastVectorRemoveAt) {
  std::vector<std::string> stringVec = {"foobar", "bar", "foo", "xyz"};
  ASSERT_EQ(stringVec.size(), 4u);
  utils::fastVectorRemoveAt(stringVec, 0);
  ASSERT_EQ(stringVec.size(), 3u);
  ASSERT_EQ(stringVec.front(), "xyz");
  ASSERT_EQ(stringVec.back(), "foo");

  // Test self-remove works properly
  utils::fastVectorRemoveAt(stringVec, 2);
  ASSERT_EQ(stringVec.size(), 2u);
  ASSERT_EQ(stringVec.front(), "xyz");
  ASSERT_EQ(stringVec.back(), "bar");

  utils::fastVectorRemoveAt(stringVec, 0);
  ASSERT_EQ(stringVec.size(), 1u);
  ASSERT_EQ(stringVec.front(), "bar");
}

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testFastVectorRemove) {
  std::vector<std::string> stringVec = {"foobar", "bar", "foo", "xyz"};
  ASSERT_EQ(stringVec.size(), 4u);
  utils::fastVectorRemove(stringVec, stringVec.begin());
  ASSERT_EQ(stringVec.size(), 3u);
  ASSERT_EQ(stringVec.front(), "xyz");
  ASSERT_EQ(stringVec.back(), "foo");

  // Test self-remove works properly
  utils::fastVectorRemove(stringVec, stringVec.end() - 1);
  ASSERT_EQ(stringVec.size(), 2u);
  ASSERT_EQ(stringVec.front(), "xyz");
  ASSERT_EQ(stringVec.back(), "bar");

  utils::fastVectorRemove(stringVec, stringVec.begin());
  ASSERT_EQ(stringVec.size(), 1u);
  ASSERT_EQ(stringVec.front(), "bar");
}

SW_NAMESPACE_END
