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
#include <sw/Buffers.h>

#include <array>

namespace sw {

TEST(BuffersTest, testUniqueBufferBasics) {
  // Basic creation. Set values to induce memory problems if they exist
  const sizex kBufSize = 10000;
  UniqueBuffer ub1 = UniqueBuffer::create(kBufSize);
  for (sizex i = 0; i < kBufSize; ++i) {
    ub1[i] = static_cast<byte>(i);
  }
  ASSERT_NE(nullptr, ub1.data());
  ASSERT_EQ(kBufSize, ub1.size());

  // Basic move
  UniqueBuffer ub2 = std::move(ub1);
  ASSERT_NE(nullptr, ub2.data());
  ASSERT_EQ(kBufSize, ub2.size());
  ASSERT_EQ(nullptr, ub1.data());
  ASSERT_EQ(0, ub1.size());

  // Swap
  const sizex kBufSize2 = 10;
  UniqueBuffer ub3 = UniqueBuffer::create(kBufSize2);
  swap(ub2, ub3);
  ASSERT_EQ(kBufSize, ub3.size());
  ASSERT_EQ(kBufSize2, ub2.size());
}

TEST(BuffersTest, testBufferWrapperBasics) {
  const sizex kBufSize = 10;
  UniqueBuffer ub1 = UniqueBuffer::create(kBufSize);
  for (sizex i = 0; i < kBufSize; ++i) {
    ub1[i] = static_cast<byte>(i);
  }

  BufferView bv1(ub1.data(), ub1.size());
  ASSERT_EQ(kBufSize, bv1.size());
  for (sizex i = 0; i < kBufSize; ++i) {
    ASSERT_EQ(ub1[i], bv1[i]);
  }
}

}  // namespace sw
