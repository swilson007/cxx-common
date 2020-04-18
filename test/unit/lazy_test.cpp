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
#include <sw/lazy.h>
#include <sw/types.h>

#include <gtest/gtest.h>

SW_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
struct TestDummy {
  int x = 0;
};

////////////////////////////////////////////////////////////////////////////////
// clang-format off
TEST(LazyTest, basic) {
  // Test 1
  {
    u32 testInc = 0;
    const auto setFunc = [&]() { ++testInc; return TestDummy{77}; };
    auto lz1 = LazyValue<TestDummy>(setFunc);
    ASSERT_EQ(0, testInc);
    ASSERT_EQ(77, lz1.get().x);
    ASSERT_EQ(1, testInc);
    ASSERT_EQ(77, lz1.get().x);
    ASSERT_EQ(1, testInc);
  }

  {
    u32 testInc = 0;
    const auto setFunc = [&]() { ++testInc; return TestDummy{77}; };
    auto lz1 = LazyLambdaValue<TestDummy, decltype(setFunc)>(setFunc);
    ASSERT_EQ(0, testInc);
    ASSERT_EQ(77, lz1.get().x);
    ASSERT_EQ(1, testInc);
    ASSERT_EQ(77, lz1.get().x);
    ASSERT_EQ(1, testInc);
  }
};
// clang-format on

SW_NAMESPACE_END
