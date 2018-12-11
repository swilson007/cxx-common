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
