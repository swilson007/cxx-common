#include <gtest/gtest.h>
#include <scw/Utils.h>

#include <array>

namespace scw {

////////////////////////////////////////////////////////////////////////////////
TEST(UtilsTest, testFormatInto) {
  constexpr sizex kBufSize = 20;
  std::array<char, kBufSize> buf = {0};
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
