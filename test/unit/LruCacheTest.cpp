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
#include <sw/lru_cache.h>

#include <gtest/gtest.h>

#include <iostream>
#include <list>
#include <string>

namespace sw {

TEST(LruCacheTest, lruBasic) {
  LruCache<int, std::string> lru;
  auto& one = lru[1];
  ASSERT_EQ("", one);
  one = "1";
  auto& oneAgain = lru[1];
  ASSERT_EQ("1", oneAgain);
  ASSERT_EQ(one, oneAgain);
}

TEST(LruCacheTest, lruPutAndGet) {
  LruCache<int, std::string> lru;
  lru.put(1, "1");
  lru.put(2, "2");
  lru.put(3, "3");
  ASSERT_EQ(3, lru.size());
  ASSERT_EQ("1", lru[1]);
  ASSERT_EQ("2", lru[2]);
  ASSERT_EQ("3", lru[3]);
  lru.put(3, "33");
  ASSERT_EQ("33", lru[3]);

  std::string str;
  ASSERT_FALSE(lru.get(4, str));
  ASSERT_TRUE(lru.get(3, str));
  ASSERT_EQ("33", str);
}

TEST(LruCacheTest, lruDelete) {
  LruCache<int, std::string> lru;
  lru[1] = "1";
  lru[2] = "2";
  lru[3] = "3";
  ASSERT_EQ(3, lru.size());
  lru.erase(2);
  ASSERT_EQ(2, lru.size());
  ASSERT_EQ("1", lru[1]);
  ASSERT_EQ("3", lru[3]);
  ASSERT_EQ("", lru[2]);
}

TEST(LruCacheTest, lruFind) {
  LruCache<int, std::string> lru;
  lru.put(1, "1");
  lru.put(2, "2");
  lru.put(3, "3");

  auto iter = lru.find(2);
  ASSERT_NE(lru.end(), iter);
  ASSERT_EQ("2", *iter);
  ASSERT_EQ("2", iter.value());
  ASSERT_EQ(2, iter.key());
}

TEST(LruCacheTest, orderedIter) {
  LruCache<int, std::string> lru;
  lru.put(3, "3");
  lru.put(2, "2");
  lru.put(1, "1");

  auto iter = lru.beginOrdered();
  ASSERT_EQ("1", *iter);
  ASSERT_EQ(1, iter.key());
  ++iter;

  ASSERT_EQ("2", *iter);
  ASSERT_EQ(2, iter.key());
  ++iter;

  ASSERT_EQ("3", *iter);
  ASSERT_EQ(3, iter.key());
  ++iter;

  ASSERT_EQ(lru.endOrdered(), iter);
}

TEST(LruCacheTest, manualPurge) {
  auto lru = LruCache<int, std::string, false>(2);
  lru.put(4, "4");
  lru.put(3, "3");
  lru.put(2, "2");
  lru.put(1, "1");
  ASSERT_EQ(4, lru.size());
  lru.purge();
  ASSERT_EQ(2, lru.size());
  auto iter = lru.beginOrdered();
  ASSERT_EQ("1", *iter++);
  ASSERT_EQ("2", *iter++);
  ASSERT_EQ(lru.endOrdered(), iter);
}

TEST(LruCacheTest, autoPurge) {
  {
    auto lru = LruCache<int, std::string, true>(2);
    lru.put(4, "4");
    lru.put(3, "3");
    lru.put(2, "2");
    ASSERT_EQ(2, lru.size());
    lru.put(1, "1");
    ASSERT_EQ(2, lru.size());
    auto iter = lru.beginOrdered();
    ASSERT_EQ("1", *iter++);
    ASSERT_EQ("2", *iter++);
    ASSERT_EQ(lru.endOrdered(), iter);
  }

  {
    auto lru = LruCache<int, std::string, true>(2);
    lru[4] = "4";
    lru[3] = "3";
    lru[2] = "2";
    ASSERT_EQ(2, lru.size());
    lru[1] = "1";
    ASSERT_EQ(2, lru.size());
    auto iter = lru.beginOrdered();
    ASSERT_EQ("1", *iter++);
    ASSERT_EQ("2", *iter++);
    ASSERT_EQ(lru.endOrdered(), iter);
  }
}

struct MoveOnly {
  ~MoveOnly() = default;
  //  ~MoveOnly() { std::cout << "~MopyOnly: ix=" << ix << ", << this=" << this << std::endl; }
  MoveOnly(int i = 0) : ix(i) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  MoveOnly(MoveOnly&& that) {
    ix = that.ix;
    that.ix = 0;
  }
  MoveOnly& operator=(MoveOnly&& that) {
    SW_ASSERT(this != &that);
    ix = that.ix;
    that.ix = 0;
    return *this;
  }

  int ix;
};

struct CopyOnly {
  ~CopyOnly() = default;
  //  ~CopyOnly() { std::cout << "~CopyOnly: ix=" << ix << ", << this=" << this << std::endl; }
  CopyOnly(int i = 0) : ix(i) {}
  CopyOnly(const CopyOnly&) = default;
  CopyOnly& operator=(const CopyOnly&) = default;
  CopyOnly(CopyOnly&&) = delete;
  CopyOnly& operator=(CopyOnly&&) = delete;
  int ix = 0;
};

TEST(LruCacheTest, verifyMoveCopyTypes) {
  {
    auto lru = LruCache<int, CopyOnly, false>(2);
    CopyOnly co{5};
    lru.put(5, co);
  }

  {
    auto lru = LruCache<int, MoveOnly, false>(2);
    lru.put(4, MoveOnly{4});
    MoveOnly mo{5};
    lru.put(5, std::move(mo));
    ASSERT_EQ(5, lru[5].ix);
    auto y = std::move(lru[5]);
    ASSERT_TRUE(lru.contains(5));
    ASSERT_EQ(5, y.ix);
    ASSERT_EQ(0, lru[5].ix);

    {
    auto iter = lru.find(4);
    ASSERT_TRUE(iter != lru.end());
    ASSERT_EQ(4, (*iter).ix);
    }

    {
      auto iter = lru.cfind(4);
      ASSERT_TRUE(iter != lru.cend());
      ASSERT_EQ(4, (*iter).ix);
    }
  }
}

TEST(LruCacheTest, moveCache) {
  auto lru = LruCache<int, std::string, true>(2);
  lru.put(2, "2");
  lru.put(1, "1");
  ASSERT_EQ(2, lru.size());

  auto lru2 = std::move(lru);
  ASSERT_EQ(0, lru.size());
  ASSERT_EQ(2, lru2.size());
  auto iter = lru2.beginOrdered();
  ASSERT_EQ("1", *iter++);
  ASSERT_EQ("2", *iter++);
  ASSERT_EQ(lru2.endOrdered(), iter);
}

TEST(LruCacheTest, copyCache) {
  auto lru = LruCache<int, std::string, true>(2);
  lru.put(2, "2");
  lru.put(1, "1");
  ASSERT_EQ(2, lru.size());

  // Copy it
  auto lru2 = lru;

  // Verify
  ASSERT_EQ(2, lru.size());
  ASSERT_EQ(2, lru2.size());
  auto iter = lru.beginOrdered();
  auto iter2 = lru2.beginOrdered();
  ASSERT_EQ(*iter++, *iter2++);
  ASSERT_EQ(*iter++, *iter2++);
  ASSERT_EQ(lru.endOrdered(), iter);
  ASSERT_EQ(lru2.endOrdered(), iter2);
}

};  // namespace sw
