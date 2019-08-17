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
#include <sw/misc.h>
#include <sw/fixed_width_int_literals.h>

#include <gtest/gtest.h>

#include <strstream>

namespace sw {

using namespace sw::intliterals;

SW_DEFINE_POD_TYPE(PodValueA, u32, ~0_u32, ~0_u32);
SW_DEFINE_POD_TYPE(PodValueB, u32, ~0_u32, ~0_u32);

////////////////////////////////////////////////////////////////////////////////
TEST(MiscTest, podWrapperTest) {
  PodValueA a = 5;
  PodValueB b = 7;

#if 0
  // These should cause syntax errors
  PodValueA ax;
  ax = b;
  u32 sum = a + 5;
  u32 sum2 = a + b;
#endif

  ASSERT_EQ(5, a);
  ASSERT_EQ(7, b);
  ASSERT_EQ(std::numeric_limits<u32>::max(), PodValueA::invalidValue());

  PodValueA a2 = 5;
  ASSERT_TRUE(a == a2);
  ASSERT_FALSE(a < a2);
  ASSERT_FALSE(a > a2);

  std::ostringstream outs;
  outs << "a=" << a << ", b=" << b;
  ASSERT_EQ("a=5, b=7", outs.str());

  ASSERT_EQ(6, ++a);
  ASSERT_EQ(6, a++);
  ASSERT_EQ(7, a);
}

}  // namespace sw
