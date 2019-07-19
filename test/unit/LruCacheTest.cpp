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
#include <list>

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

}  // namespace sw
