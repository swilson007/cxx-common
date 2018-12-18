#include <gtest/gtest.h>
#include <scw/Strings.h>
#include <scw/Uuid.h>

#include <iostream>
#include <sstream>

namespace scw {

////////////////////////////////////////////////////////////////////////////////
TEST(UuidTest, testToString) {
  std::array<u8, 16> u1data = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255}};

  auto u0 = Uuid();
  ASSERT_FALSE(u0.isValid());

  auto u1 = Uuid(u1data);
  ASSERT_TRUE(u1.isValid());

  auto u1s = u1.toString();
  ASSERT_STREQ("01020304-0506-0708-090a-0b0c0d0e0fff", u1s.c_str());
}

////////////////////////////////////////////////////////////////////////////////
TEST(UuidTest, testInvalid) {
  std::array<u8, 16> u1data = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255}};

  auto u0 = Uuid();
  ASSERT_FALSE(u0.isValid());

  auto u1 = Uuid();
  ASSERT_FALSE(u1.isValid());

  auto u1s = u1.toString();
  ASSERT_STREQ("00000000-0000-0000-0000-000000000000", u1s.c_str());
  ASSERT_EQ(u0, u1);
}

////////////////////////////////////////////////////////////////////////////////
TEST(UuidTest, testOutput) {
  auto u0 = Uuid();
  std::cout << "empty uuid=" << u0 << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
TEST(UuidTest, testCreate) {
  auto u0 = Uuid::create();
  std::cout << "real uuid=" << u0 << std::endl;
}

}  // namespace scw
