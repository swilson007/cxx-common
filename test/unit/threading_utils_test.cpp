////////////////////////////////////////////////////////////////////////////////
/// Copyright 2018 Steven C. Wilson
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
/// associated documentation files (the "Software"), to deal in the Software without restriction, including
/// without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
/// following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or substantial
/// portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
/// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
/// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
/// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////
#include <sw/threading_utils.h>

#include <gtest/gtest.h>

#include <mutex>

namespace sw {

std::mutex gTestMutex;

////////////////////////////////////////////////////////////////////////////////
TEST(VersionedValueCacheTest, basic) {
  VersionedValueCache<std::string, std::mutex, std::lock_guard<std::mutex>> stringValueCache(gTestMutex);
  std::string current = "Hello World";
  // Copy current and put it into the cached value so we can use 'current' for testing
  stringValueCache.setValue(std::string{current});
  ASSERT_EQ(0, stringValueCache.copyCount());

  // Checkout and explicit checkin
  {
    auto valueCopy = stringValueCache.checkout();
    ASSERT_EQ(0, stringValueCache.copyCount());
    ASSERT_EQ(current, valueCopy.value());
    stringValueCache.checkin(std::move(valueCopy));
    ASSERT_EQ(1, stringValueCache.copyCount());
  }

  // Checkout and auto-checkin
  {
    auto valueCopy = stringValueCache.checkout();
    ASSERT_EQ(0, stringValueCache.copyCount());
    {
      auto valueCopy2 = stringValueCache.checkout();
      ASSERT_EQ(0, stringValueCache.copyCount());
      ASSERT_EQ(current, valueCopy2.value());
    }
    ASSERT_EQ(1, stringValueCache.copyCount());
    ASSERT_EQ(current, valueCopy.value());
  }
  ASSERT_EQ(2, stringValueCache.copyCount());

  // Set a new value, thus changing the version
  current = "Hello World v2";
  stringValueCache.setValue(std::string{current});

  // All copies should have been cleared
  ASSERT_EQ(0, stringValueCache.copyCount());
  {
    auto valueCopy = stringValueCache.checkout();
    ASSERT_EQ(current, valueCopy.value());
  }
  ASSERT_EQ(1, stringValueCache.copyCount());

  // Set a new value, thus changing the version
  current = "Hello World v3";
  stringValueCache.setValue(std::string{current}, 2);
  ASSERT_EQ(2, stringValueCache.copyCount());

  {
    auto valueCopy = stringValueCache.checkout();
    ASSERT_EQ(1, stringValueCache.copyCount());
    auto valueCopy2 = stringValueCache.checkout();
    ASSERT_EQ(0, stringValueCache.copyCount());
    ASSERT_EQ(current, valueCopy.value());
    ASSERT_EQ(current, valueCopy2.value());
  }
  ASSERT_EQ(2, stringValueCache.copyCount());
}

////////////////////////////////////////////////////////////////////////////////
TEST(VersionedValueCacheTest, atomicSharedValue) {
  std::mutex testMutex;
  AtomicSharedValue<std::string, std::mutex, std::lock_guard<std::mutex>> sharedString(testMutex);
  std::string hello = "Hello World";

  sharedString.set(std::make_unique<std::string>(hello));
  auto getV1 = sharedString.get();
  ASSERT_EQ(hello, *getV1);

  {
    auto getV2 = sharedString.get();
    ASSERT_EQ(hello, *getV2);
    ASSERT_EQ(*getV1, *getV2);
  }

  // Set the main value to something else
  std::string goodbye = "Goodbye World";
  sharedString.set(std::make_unique<std::string>(goodbye));

  // Ensure our original value is unaffected
  ASSERT_EQ(hello, *getV1);

  auto getV3 = sharedString.get();
  ASSERT_EQ(goodbye, *getV3);
  ASSERT_NE(*getV1, *getV3);
}

}  // namespace sw
