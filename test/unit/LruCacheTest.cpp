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
#include <string>

namespace sw {

TEST(LruCacheTest, pushFront) {
  lru_detail::List<int> list;
  list.pushFront(3);
  list.pushFront(2);
  list.pushFront(1);

  {
    auto iter = list.begin();
    ASSERT_EQ(1, *iter++);
    ASSERT_EQ(2, *iter++);
    ASSERT_EQ(3, *iter++);
    ASSERT_EQ(iter, list.end());
  }
}

TEST(LruCacheTest, pushBack) {
  lru_detail::List<int> list;
  list.pushBack(1);
  list.pushBack(2);
  list.pushBack(3);

  {
    auto iter = list.begin();
    ASSERT_EQ(1, *iter++);
    ASSERT_EQ(2, *iter++);
    ASSERT_EQ(3, *iter++);
    ASSERT_EQ(iter, list.end());
  }
}

TEST(LruCacheTest, moveNodes) {
  lru_detail::List<int> list;
  list.pushBack(1);
  auto* node2 = list.pushBack(2);
  list.pushBack(3);
  list.moveToBack(node2);
  {
    auto iter = list.begin();
    ASSERT_EQ(1, *iter++);
    ASSERT_EQ(3, *iter++);
    ASSERT_EQ(2, *iter++);
    ASSERT_EQ(iter, list.end());
  }

  list.moveToFront(node2);
  {
    auto iter = list.begin();
    ASSERT_EQ(2, *iter++);
    ASSERT_EQ(1, *iter++);
    ASSERT_EQ(3, *iter++);
    ASSERT_EQ(iter, list.end());
  }
}

TEST(LruCacheTest, lruBasic) {
  LruCache<int, std::string> lru;
  auto& one = lru[1];
  ASSERT_EQ("", one);
  one = "1";
  auto& oneAgain = lru[1];
  ASSERT_EQ("1", oneAgain);
  ASSERT_EQ(one, oneAgain);
}

TEST(LruCacheTest, lruDelete) {
  LruCache<int, std::string> lru;
  lru[1] = "1";
  lru[2] = "2";
  lru[3] = "3";
  ASSERT_EQ(3, lru.size());
  lru.erase(2);
  ASSERT_EQ(2, lru.size());
}

}  // namespace sw
