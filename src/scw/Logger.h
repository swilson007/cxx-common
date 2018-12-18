#pragma once

#include "HiResTimer.h"
#include "Strings.h"
#include "System.h"

#include <iostream>
#include <mutex>
#include <sstream>

namespace scw {

////////////////////////////////////////////////////////////////////////////////
/// This code is quick and minimally featured. Once I need real logging, consider
/// digging around for something already built and good.
////////////////////////////////////////////////////////////////////////////////

template <typename System = system::ThisSystem>
class LoggerType;
using Logger = LoggerType<>;

////////////////////////////////////////////////////////////////////////////////
template <typename System>
class LoggerType {
public:
  static constexpr sizex kMaxLogMessageLength = 2048u;

  enum class Category {
    kDebug = 0,
    kInfo = 1,
    kWarn = 2,
  };

  ////////////////////////////////////////////////////////////////////////////////
  static const char* toString(Category category) {
    switch (category) {
    case Category::kDebug:
      return "dbg ";
    case Category::kWarn:
      return "warn";
    case Category::kInfo:
      return "info";
    }
    return "????";
  }

  LoggerType() : lout_(&logIoBuf_) {}
  ~LoggerType() { std::cout.flush(); }

  LoggerType(const LoggerType&) = delete;
  LoggerType& operator=(const LoggerType&) = delete;
  LoggerType(LoggerType&&) = delete;
  LoggerType& operator=(LoggerType&&) = delete;

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a message at the specified category
  void log(Category category, const StringWrapper& msg) {
    // We output a high-res timestamp that's a delta based on our start time
    char logBuffer[kMaxLogMessageLength];
    Utils::formatInto(logBuffer, kMaxLogMessageLength, "%11.4lf:%4s: %s", toString(category),
                      hrt_.elapsedSecs().count(), msg.c_str());

    // Locking just around the actual output means messages can print out-of-order
    MutexLock lock(lock_);
    std::cout << logBuffer << std::endl;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message at the specified category
  template <typename... Ts>
  void logf(Category cat, const StringWrapper& format, Ts... ts) {
    char messageBuffer[kMaxLogMessageLength];
    Utils::formatInto(messageBuffer, kMaxLogMessageLength, format, std::forward<Ts>(ts)...);
    log(cat, messageBuffer);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a debug message
  void debug(const StringWrapper& s) { log(Category::kDebug, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the DEBUG category
  template <typename... Ts>
  void debugf(const StringWrapper& format, Ts... ts) {
    logf(Category::kDebug, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an info message
  void info(const StringWrapper& msg) { log(Category::kInfo, msg); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the INFO category
  template <typename... Ts>
  void infof(const StringWrapper& format, Ts... ts) {
    logf(Category::kInfo, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a warning message
  void warn(const StringWrapper& s) { log(Category::kWarn, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log the exception with the WARN category
  void warn(const std::exception& e) { log(Category::kWarn, e.what()); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the WARN category
  template <typename... Ts>
  void warnf(const StringWrapper& format, Ts... ts) {
    logf(Category::kWarn, format, std::forward<Ts>(ts)...);
  }

  std::ostream& lout() { return lout_; }

private:
  /// SCW: Very simplistic support for stream based log. This should log whatever is written to
  /// lout whenever it is flushed. Use like:
  ///  logger().lout() << "foo" << std::flush;
  /// OR
  ///  logger().lout() << "foo" << std::endl;
  /// The latter case works in that this code strips a trailing '\n' if found.  (remember that
  /// std::endl flushes)

  class LogIoBuf : public std::stringbuf {
  public:
    LogIoBuf() : std::stringbuf(std::ios_base::out) {}

  protected:
    virtual int sync() override {
      auto result = std::stringbuf::sync();
      auto message = str();
      if (!message.empty()) {
        if (message.back() == '\n') {
          message.pop_back();
        }
        logger().info(message);
        str("");
      }

      return result;
    }
  };

  LogIoBuf logIoBuf_;
  std::ostream lout_;  // (&logIoBuf_);

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// Support for a global logger
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// I don't love any of this, but it should work for now
  static void createLogger() {
    gLogger_ = std::make_unique<LoggerType>();
    gLogger_->info("create-logger");
  }

  ////////////////////////////////////////////////////////////////////////////////
  static void destroyLogger() {
    gLogger_->info("destroy-logger");
    gLogger_.reset();
  }

  ////////////////////////////////////////////////////////////////////////////////
  static LoggerType& logger() {
    Assert(gLogger_ != nullptr);
    return *gLogger_;
  }

  static std::unique_ptr<LoggerType> gLogger_;

private:
  std::mutex lock_;
  const HiResTimer hrt_;

  friend void createLogger();
  friend void destroyLogger();
  friend LoggerType& logger();
};

template <typename System>
std::unique_ptr<LoggerType<System>> LoggerType<System>::gLogger_;

////////////////////////////////////////////////////////////////////////////////
inline void createLogger() {
  Logger::createLogger();
}
inline void destroyLogger() {
  Logger::destroyLogger();
}
inline Logger& logger() {
  return Logger::logger();
}

////////////////////////////////////////////////////////////////////////////////
/// RAII for create/destroy logger. Useful for unit tests
class CreateAndDestroyLogger : public NonCopyable, NonMovable {
public:
  CreateAndDestroyLogger() { createLogger(); }
  ~CreateAndDestroyLogger() { destroyLogger(); }
};

}  // namespace sw