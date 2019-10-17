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
#include <sw/logger.h>

#include <gtest/gtest.h>

namespace sw {

struct TestLogEntry {
  LoggerCategory cat = LoggerCategory::None;
  std::string msg;
  bool force = false;
};

////////////////////////////////////////////////////////////////////////////////
struct TestLogHandler : LogHandler {
  void onLog(SystemTimepoint logTime, LoggerCategory cat, const StringWrapper& msg, bool force) override {
      unused(logTime);
      entries.emplace_back(TestLogEntry{cat, std::string{msg.data(), msg.size()}, force});
  };

  std::vector<TestLogEntry> entries;
};

////////////////////////////////////////////////////////////////////////////////
TEST(LoggerTest, basic) {
    auto handler = std::make_shared<TestLogHandler>();
    Logger logger(handler);
    logger.info("hello");
    logger.debugf("hello={}", "goodbye");

    ASSERT_EQ(2, handler->entries.size());

    ASSERT_EQ(std::string{"hello"}, handler->entries[0].msg);
    ASSERT_EQ(LoggerCategory::Info, handler->entries[0].cat);
    ASSERT_EQ(false, handler->entries[0].force);

    ASSERT_EQ(std::string{"hello=goodbye"}, handler->entries[1].msg);
    ASSERT_EQ(LoggerCategory::Debug, handler->entries[1].cat);
    ASSERT_EQ(false, handler->entries[1].force);
}

}  // namespace sw
