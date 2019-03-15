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
#include <sw/Strings.h>
#include <sw/Uuid.h>

#include <iostream>
#include <sstream>

namespace sw {

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

}  // namespace sw
