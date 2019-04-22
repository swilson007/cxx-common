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
#include <sw/hi_res_timer.h>
#include <sw/paged_buffer.h>
#include <sw/strings.h>

#include <gtest/gtest.h>

#include <array>

namespace sw {

////////////////////////////////////////////////////////////////////////////////
TEST(PagedBufferTest, basicTest) {
  auto buffer = PagedBuffer<4>(0);
  ASSERT_EQ(buffer.size(), 0);
  const std::array<byte, 1024> sourceBuffer = {0, 1, 2, 3, 4, 5};
  sizex pos = 0;
  buffer.copyInto(sourceBuffer.data(), 6);
  ASSERT_EQ(buffer.size(), 6);
  ASSERT_TRUE(buffer.capacity() > buffer.size());
  ASSERT_EQ(buffer[0], 0);
  ASSERT_EQ(buffer[1], 1);
  ASSERT_EQ(buffer[5], 5);

  buffer.copyInto(sourceBuffer.data(), 6);
  ASSERT_EQ(buffer.size(), 12);
  ASSERT_EQ(buffer[6], 0);
  ASSERT_EQ(buffer[7], 1);
  ASSERT_EQ(buffer[11], 5);

  std::array<byte, 1024> destBuffer;
  buffer.copyFrom(0, destBuffer.data(), 12);
  ASSERT_EQ(destBuffer[0], 0);
  ASSERT_EQ(destBuffer[1], 1);
  ASSERT_EQ(destBuffer[5], 5);
  ASSERT_EQ(destBuffer[6], 0);
  ASSERT_EQ(destBuffer[7], 1);
  ASSERT_EQ(destBuffer[11], 5);
}

}  // namespace sw
