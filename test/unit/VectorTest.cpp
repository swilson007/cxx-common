#include <gtest/gtest.h>
#include <scw/HiResTimer.h>
#include <scw/Vector.h>

#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace scw {

struct TrackedItem {
  ~TrackedItem() { --sItems; }
  TrackedItem() { ++sItems; }
  TrackedItem(const TrackedItem& that) { ++sItems; }
  TrackedItem(TrackedItem&& that) { ++sItems; }

  static int sItems;
};
int TrackedItem::sItems = 0;

TEST(VectorTest, testCtor) {
  std::vector<u64> stdvec;
  stdvec.resize(10);
  stdvec.reserve(10);

  scw::Vector<int> ivec;
  ivec.reserve(1);
  ivec.push_back(1);
  ivec.push_back(2);
}

TEST(VectorTest, testCopy) {
  Vector<int> vec;
  vec.reserve(10);
  vec.push_back(1);
  vec.push_back(2);

  Vector<int> vec2 = vec;
  ASSERT_EQ(2, vec.size());
  ASSERT_EQ(vec.size(), vec2.size());
  ASSERT_EQ(10, vec.capacity());
  ASSERT_EQ(vec.capacity(), vec2.capacity());
  ASSERT_EQ(1, vec[0]);
  ASSERT_EQ(1, vec2[0]);
}

TEST(VectorTest, testMove) {
  Vector<int> vec;
  vec.reserve(10);
  vec.push_back(1);
  vec.push_back(2);
  Vector<int> vec2 = std::move(vec);

  ASSERT_EQ(2, vec2.size());
  ASSERT_EQ(10, vec2.capacity());
  ASSERT_NE(nullptr, vec2.data());
  ASSERT_EQ(1, vec2[0]);
  ASSERT_EQ(0, vec.size());
  ASSERT_EQ(0, vec.capacity());
  ASSERT_EQ(nullptr, vec.data());
}

TEST(VectorTest, testResize) {
  TrackedItem::sItems = 0;
  Vector<TrackedItem> vec;
  ASSERT_EQ(0, TrackedItem::sItems);
  vec.reserve(2);
  ASSERT_EQ(0, TrackedItem::sItems);
  vec.resize(1);
  ASSERT_EQ(1, TrackedItem::sItems);
  vec.resize(5);
  ASSERT_EQ(5, TrackedItem::sItems);
  vec.resize(3);
  ASSERT_EQ(3, TrackedItem::sItems);
  vec.clear();
  ASSERT_EQ(0, TrackedItem::sItems);
}

TEST(VectorTest, testGetters) {
  Vector<u32> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  ASSERT_EQ(1, vec.front());
  ASSERT_EQ(3, vec.back());
  ASSERT_EQ(1, vec.at(0));
  ASSERT_EQ(2, vec.at(1));
  ASSERT_EQ(3, vec.at(2));
  vec.at(0) = 5;
  ASSERT_EQ(5, vec.at(0));
}

TEST(VectorTest, testInitList) {
  Vector<i32> vec = {1, 2, 3};
  ASSERT_EQ(3, vec.size());
  ASSERT_EQ(1, vec.front());
  ASSERT_EQ(3, vec.back());
  ASSERT_EQ(1, vec.at(0));
  ASSERT_EQ(2, vec.at(1));
  ASSERT_EQ(3, vec.at(2));
}

TEST(VectorTest, testIters) {
  {
    TrackedItem::sItems = 0;
    Vector<TrackedItem> vec;
    vec.resize(10, TrackedItem());
    size_t count = 0;
    for (const auto& TrackedItem : vec) {
      ++count;
    }
    ASSERT_EQ(10, count);
  }
  ASSERT_EQ(0, TrackedItem::sItems);
}

template <typename T>
sizex doVectorPerf(sizex iterations) {
  sizex count = 0;
  for (sizex iter = 0; iter < iterations; ++iter) {
    T s;
    s.reserve(20);
    for (size_t i = 0; i < 20; ++i) {
      s.push_back(i);
    }
    count += s.capacity();
    count += s[0];

    s.reserve(45);
    for (size_t i = 0; i < 20; ++i) {
      s.push_back(i);
    }
    count += s.capacity();
    count += s[0];

    s.reserve(100);
    count += s.capacity();
  }
  return count;
}

#define TEST_PERFORMANCE 1
#if TEST_PERFORMANCE
TEST(VectorTest, vecPerf) {
#  if SCW_DEBUG
  const sizex kIterations = 1;
  const sizex kTestIterations = 1;
#  else
  const sizex kIterations = 6000000;
  const sizex kTestIterations = 5;
#  endif
  for (sizex test = 0; test < kTestIterations; ++test) {
#  if SCW_RELEASE
    {
      HiResTimer hrt;
      auto count = doVectorPerf<std::vector<u64>>(kIterations);
      auto elapsed = hrt.elapsedMs();
      std::cout << "std::vector=" << elapsed.count() << "ms count=" << count << std::endl;
    }
#  endif

    {
      HiResTimer hrt;
      auto count = doVectorPerf<scw::Vector<u64>>(kIterations);
      auto elapsed = hrt.elapsedMs();
      std::cout << "scw::Vector=" << elapsed.count() << "ms count=" << count << std::endl;
    }
  }
}
#endif

static std::mt19937 gRandEngine;
static std::uniform_int_distribution<size_t> gRandDist(0, std::numeric_limits<size_t>::max());

// Non-trivail with an expensive default ctor
struct NonTrivialFoo {
  static size_t sId;
  static size_t sCtors;
  static size_t sDtors;
  static size_t sCopies;
  static size_t sMoves;
  static size_t sActive;
  NonTrivialFoo() : id(sId++), rand(gRandDist(gRandEngine)) {
#if SCW_DEBUG
    std::cout << "NTF: ctor: this=" << this << std::endl;
#endif
    rand2 = gRandDist(gRandEngine);
    ++sActive;
    ++sCtors;
  }

  NonTrivialFoo(const NonTrivialFoo& that) noexcept {
#if SCW_DEBUG
    std::cout << "NTF: copy-ctor: this=" << this << std::endl;
#endif
    this->id = sId++;
    this->rand = that.rand;
    this->rand2 = that.rand2;
    ++sActive;
    ++sCopies;
  }

  NonTrivialFoo(NonTrivialFoo&& that) noexcept {
#if SCW_DEBUG
    std::cout << "NTF: move-ctor: this=" << this << std::endl;
#endif
    this->id = sId++;
    this->rand = std::exchange(that.rand, 0);
    this->rand2 = std::exchange(that.rand2, 0);
    ++sActive;
    ++sMoves;
  }

  virtual ~NonTrivialFoo() noexcept {
#if SCW_DEBUG
    std::cout << "NTF: dtor: this=" << this << std::endl;
#endif
    --sActive;
    ++sDtors;
  }
  virtual int notTrivial() { return 1; }

  size_t id;
  size_t rand;
  size_t rand2;
};
size_t NonTrivialFoo::sId = 1;
size_t NonTrivialFoo::sCtors = 0;
size_t NonTrivialFoo::sDtors = 0;
size_t NonTrivialFoo::sCopies = 0;
size_t NonTrivialFoo::sMoves = 0;
size_t NonTrivialFoo::sActive = 0;

TEST(VectorTest, nonTrivialVec) {
  Vector<NonTrivialFoo> foo5(1);
  Vector<NonTrivialFoo> foo520(1, 20);
  auto copy1 = foo5;
  auto copy2 = foo5;
}

template <typename T>
sizex doNonTrivialVectorPerf(sizex iterations) {
  sizex count = 0;
  for (sizex iters = 0; iters < iterations; ++iters) {
    T s(20);
    count += s.capacity();

    s.reserve(45);
    for (size_t i = 0; i < 20; ++i) {
      s.push_back(NonTrivialFoo{});
    }
    count += s.capacity();

    s.reserve(100);
    count += s.capacity();
  }
  return count;
}

#if TEST_PERFORMANCE
TEST(VectorTest, nonTrivialVecPerf) {
#  if SCW_DEBUG
  const sizex kIterations = 1;
  const sizex kTestIterations = 1;
#  else
  const sizex kIterations = 1000000;
  const sizex kTestIterations = 5;
#  endif
  for (sizex test = 0; test < kTestIterations; ++test) {
    {
      HiResTimer hrt;
      auto count = doNonTrivialVectorPerf<std::vector<NonTrivialFoo>>(kIterations);
      auto elapsed = hrt.elapsedMs();
      std::cout << "std::vector=" << elapsed.count() << "ms count=" << count << std::endl;
      std::cout << " ctors=" << NonTrivialFoo::sCtors << ", copies=" << NonTrivialFoo::sCopies
                << ", moves=" << NonTrivialFoo::sMoves << ", dtors=" << NonTrivialFoo::sDtors
                << std::endl;
      NonTrivialFoo::sCtors = 0;
      NonTrivialFoo::sDtors = 0;
      NonTrivialFoo::sCopies = 0;
      NonTrivialFoo::sMoves = 0;
    }

    ASSERT_EQ(0, NonTrivialFoo::sActive);

    {
      HiResTimer hrt;
      auto count = doNonTrivialVectorPerf<scw::Vector<NonTrivialFoo>>(kIterations);
      auto elapsed = hrt.elapsedMs();
      std::cout << "scw::Vector=" << elapsed.count() << "ms count=" << count << std::endl;
      std::cout << " ctors=" << NonTrivialFoo::sCtors << ", copies=" << NonTrivialFoo::sCopies
                << ", moves=" << NonTrivialFoo::sMoves << ", dtors=" << NonTrivialFoo::sDtors
                << std::endl;
      NonTrivialFoo::sCtors = 0;
      NonTrivialFoo::sDtors = 0;
      NonTrivialFoo::sCopies = 0;
      NonTrivialFoo::sMoves = 0;
    }

    ASSERT_EQ(0, NonTrivialFoo::sActive);
  }
}
#endif

}  // namespace scw
