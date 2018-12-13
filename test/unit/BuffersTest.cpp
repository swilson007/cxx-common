#include <gtest/gtest.h>
#include <scw/Buffers.h>

#include <array>

namespace scw {

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

}  // namespace scw::foo
