////////////////////////////////////////////////////////////////////////////////
/// Copyright 2019 Steven C. Wilson
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
#pragma once

#include "strings.h"
#include "system_traits.h"
#include "utils.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>

namespace sw {

////////////////////////////////////////////////////////////////////////////////
/// This code is quick and minimally featured. I had a plan to one day use a real
/// logger, but after looking at the options, decided to just keep using this. The
/// front end is nice, the backend could use some love for real applications.
////////////////////////////////////////////////////////////////////////////////
template <typename SystemTraits = system::ThisSystemTraits>
class LoggerType;
using Logger = LoggerType<>;

////////////////////////////////////////////////////////////////////////////////
enum class LoggerTimeStyle : uint8 { None, Delta, Absolute };

////////////////////////////////////////////////////////////////////////////////
inline char const* toString(LoggerTimeStyle v) {
  // clang-format off
    switch (v) {
        case LoggerTimeStyle::None: return "none";
        case LoggerTimeStyle::Delta: return "delta";
        case LoggerTimeStyle::Absolute: return "absolute";
    }
  // clang-format on
  return "unknown";
}

////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator<<(std::ostream& outs, LoggerTimeStyle v) {
  outs << toString(v);
  return outs;
}

////////////////////////////////////////////////////////////////////////////////
enum class LoggerConsoleDestination : uint8 {
  None,
  Stdout,
  Stderr,
};

////////////////////////////////////////////////////////////////////////////////
/// Messages are logged at a specific level, but we support a mask for determining
/// what gets displayed thus the bitmask.
enum class LoggerCategory : uint8 {
  None = 0,
  Error = 1u << 0,
  Warn = 1u << 1,
  Info = 1u << 2,
  Verbose = 1u << 3,
  Debug = 1u << 4,

  All = 0xff
};
SW_DEFINE_ENUM_BITFIELD_OPERATORS(LoggerCategory);

////////////////////////////////////////////////////////////////////////////////
/// This is the "backend" for the logger. Implement to do as needed.
///
/// The virtual approach seems old school, but will outperform using std::function.
struct LogHandler {
  virtual ~LogHandler() = default;
  virtual void onLog(SystemTimepoint logTime, LoggerCategory cat, const StringWrapper& msg, bool force) = 0;
};
using LogHandlerRef = std::shared_ptr<LogHandler>;

////////////////////////////////////////////////////////////////////////////////
/// Defines a fairly simplistic logger designed to use the very good {fmt} library
/// for formatted strings.
/// The general design is a front-end/back-end approach. This class is the front
/// end, where the back end is whatever instance of LogHandler that gets attached.
/// A few backends are included at the end of this file.
template <typename SystemTraits>
class LoggerType {
public:
  using Category = LoggerCategory;

public:
  ////////////////////////////////////////////////////////////////////////////////
  static bool canLogCategory(LoggerCategory cat, Category mask, bool force) {
    // If the mask is fully disabled, we never log, even with force
    bool result = (mask == Category::None) ? false : (force || (cat & mask) == cat);
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Return four-character code for the given category
  static char const* categoryCode(Category category) {
    switch (category) {
    case Category::Debug:
      return "dbug";
    case Category::Verbose:
      return "verb";
    case Category::Warn:
      return "warn";
    case Category::Info:
      return "info";
    case Category::Error:
      return "erro";

    // These aren't intended to be used, but we put them in here to make the compiler happy
    case Category::None:
      return "none";
    case Category::All:
      return "all_";
    }
    return "????";
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates an unusable logger
  LoggerType() = default;

  ////////////////////////////////////////////////////////////////////////////////
  LoggerType(const LogHandlerRef& handler) noexcept : _logHandler(handler) {}

  ////////////////////////////////////////////////////////////////////////////////
  ~LoggerType() = default;

  // No move/copy
  LoggerType(LoggerType const&) = delete;
  LoggerType& operator=(LoggerType const&) = delete;
  LoggerType(LoggerType&&) = default;
  LoggerType& operator=(LoggerType&&) = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Get the starting timepoint
  SystemTimepoint getStartTimepoint() const { return _startTime; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a message at the specified category
  void log(SystemTimepoint logTime, Category category, StringWrapper const& msg, bool force = false) {
    _logHandler->onLog(logTime, category, msg, force);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a message at the specified category
  void log(Category category, StringWrapper const& msg, bool force = false) {
    _logHandler->onLog(SystemClock::now(), category, msg, force);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Force a log entry using {fmt} style formatted message at the specified category
  /// This overrides any category mask disables.
  template <typename... Ts>
  void logForcef(Category cat, StringWrapper const& format, Ts... ts) {
    std::string logString = fmt::format(format.c_str(), std::forward<Ts>(ts)...);
    log(cat, logString, true);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an {fmt} style formatted message at the specified category
  template <typename... Ts>
  void logf(Category cat, StringWrapper const& format, Ts... ts) {
    std::string logString = fmt::format(format.c_str(), std::forward<Ts>(ts)...);
    log(cat, logString);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a trace message
  void verbose(StringWrapper const& s) { log(Category::Verbose, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the Trace category
  template <typename... Ts>
  void verbosef(StringWrapper const& format, Ts... ts) {
    logf(Category::Verbose, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a debug message
  void debug(StringWrapper const& s) { log(Category::Debug, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the Debug category
  template <typename... Ts>
  void debugf(StringWrapper const& format, Ts... ts) {
    logf(Category::Debug, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an info message
  void info(StringWrapper const& msg) { log(Category::Info, msg); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the Info category
  template <typename... Ts>
  void infof(StringWrapper const& format, Ts... ts) {
    logf(Category::Info, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log a warning message
  void warn(StringWrapper const& s) { log(Category::Warn, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log the exception with the Warn category
  void warn(std::exception const& e) { log(Category::Warn, e.what()); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the WARN category
  template <typename... Ts>
  void warnf(StringWrapper const& format, Ts... ts) {
    logf(Category::Warn, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an error message
  void error(StringWrapper const& s) { log(Category::Error, s); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log the exception with the Error category
  void error(std::exception const& e) { log(Category::Error, e.what()); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Log an sprintf style formatted message with the Error category
  template <typename... Ts>
  void errorf(StringWrapper const& format, Ts... ts) {
    logf(Category::Error, format, std::forward<Ts>(ts)...);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Support for a global logger
  ////////////////////////////////////////////////////////////////////////////////

private:
  /// Note - Using 'system' clock so that it's convertable to time_t and capable of date formatting.
  /// TODO: C++20 solves this and allows use of steady and hi-res clocks
  SystemTimepoint _startTime;

  LogHandlerRef _logHandler;
};

////////////////////////////////////////////////////////////////////////////////
/// Simple empty logger
struct NullLogHandler : public LogHandler {
  virtual ~NullLogHandler() = default;
  virtual void onLog(SystemTimepoint logTime, Logger::Category cat, const StringWrapper& msg,
                     bool force) override {
    unused(logTime);
    unused(cat);
    unused(msg);
    unused(force);
    nop();
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Simple delta-time console logger
struct SimpleConsoleLogHandler : public LogHandler {
  using Category = Logger::Category;

  ////////////////////////////////////////////////////////////////////////////////
  void onLog(SystemTimepoint logTime, Logger::Category cat, const StringWrapper& msg, bool force) {
    // Ignore item if category if it's disabled
    if (!Logger::canLogCategory(cat, _categoryMask, force)) {
      return;
    }

    auto const& elapsed = logTime - _startTime;
    auto const& elapsedSecs = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed);
    auto logStr = fmt::format("{:.3f}:{:4}: {}", elapsedSecs.count(), Logger::categoryCode(cat), msg.c_str());

    // Note that it's possible for items to be logged out of time order
    {
      MutexLock lock(_lock);
      std::cout << logStr << std::endl;
    }
  }

private:
  SystemTimepoint _startTime = SystemClock::now();
  Category _categoryMask = Category::Info | Category::Warn | Category::Error;
  std::mutex _lock;
};

////////////////////////////////////////////////////////////////////////////////
/// Beefier log-handler that can log to console, file, and/or callback
struct ConsoleFileLogHandler : public LogHandler {
  using Category = Logger::Category;
  using ConsoleDestination = LoggerConsoleDestination;

  ////////////////////////////////////////////////////////////////////////////////
  struct Config {
    std::string logFile;  ///> Use "" for no log file
    LoggerTimeStyle fileTimeStyle = LoggerTimeStyle::Absolute;
    Category fileCategoryMask = Category::All;

    // Log to console support
    Category consoleCategoryMask = Category::All;
    LoggerTimeStyle consoleTimeStyle = LoggerTimeStyle::Delta;
    ConsoleDestination console_destination = ConsoleDestination::Stdout;
  };

  ConsoleFileLogHandler(Config config) : _config(config) {}

  void onLog(SystemTimepoint logTime, Logger::Category cat, const StringWrapper& msg, bool force) override;

private:
  SystemTimepoint _startTime = SystemClock::now();
  Config _config;
  std::ofstream _fout;  ///> File to log to. Will be unused when file logging is disabled
  std::mutex _lock;
};

////////////////////////////////////////////////////////////////////////////////
/// Asynchronous log handler
struct AsyncLogHandler : public LogHandler {
  struct LogEntry {
    LogEntry() = default;
    LogEntry(SystemTimepoint time, Logger::Category catx, std::string&& msgx, bool forcex) :
        logTime(time),
        cat(catx),
        msg(std::move(msgx)),
        force(forcex) {}
    SystemTimepoint logTime;
    Logger::Category cat;
    std::string msg;
    bool force;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ~AsyncLogHandler() {
    // Let our thread know it's exit time
    {
      std::lock_guard<decltype(_queueLock)> lockGuard(_queueLock);
      _exit = true;
      _drain = false;
    }
    _queueCondition.notify_all();

    // Wait for the thread to exit
    try {
      if (_thread.joinable()) {
        _thread.join();
      }
    } catch (...) {
      SW_ASSERT(false);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  AsyncLogHandler(LogHandlerRef forwardLogger) :
      _targetLogger(std::move(forwardLogger)),
      _thread([this]() { this->threadExec(); }) {}

  ////////////////////////////////////////////////////////////////////////////////
  void onLog(SystemTimepoint logTime, Logger::Category cat, const StringWrapper& msg, bool force) {
    // Put log entry on queue
    {
      std::lock_guard<decltype(_queueLock)> lockGuard(_queueLock);

      // Don't add new log items once we've exited
      if (_exit) {
        // Somebody logged after the logger shutdown
        SW_ASSERT(false);
      } else {
        _queue.emplace(logTime, cat, std::string(msg.data(), msg.size()), force);
      }
    }

    _queueCondition.notify_one();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Safe shutdown that drains the queue before exiting
  void shutdown() {
    {
      std::lock_guard<decltype(_queueLock)> lockGuard(_queueLock);
      _drain = true;  // Order matters here
      _exit = true;
    }
    _queueCondition.notify_all();

    // Wait for the thread to exit
    if (_thread.joinable()) {
      _thread.join();
    }
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// Thread execution function for the async logger. This delivers all log messages
  /// to our forwarding thread, and thus they all come in on the same thread to the
  /// target logger.
  void threadExec() {
    LogEntry logEntry;
    while (true) {
      {
        std::unique_lock<decltype(_queueLock)> lock(_queueLock);
        _queueCondition.wait(lock, [&]() { return _exit || !_queue.empty(); });
        if (_exit && (!_drain || _queue.empty())) {
          return;
        }

        // Process the next log item
        SW_ASSERT(!_queue.empty());
        logEntry = std::move(_queue.front());
        _queue.pop();
      }

      // Forward the log entry, do it unlocked!
      try {
        _targetLogger->onLog(logEntry.logTime, logEntry.cat, logEntry.msg, logEntry.force);
      } catch (const std::exception& ex) {
        SW_ASSERT(false);
      }
    }
  }

  LogHandlerRef _targetLogger;

  std::thread _thread;
  std::queue<LogEntry> _queue;
  std::mutex _queueLock;
  std::condition_variable _queueCondition;
  bool _exit = false;   ///> Does not need to be atomic since it's guarded by a mutex
  bool _drain = false;  ///> Does not need to be atomic since it's guarded by a mutex
};

namespace log_detail {

////////////////////////////////////////////////////////////////////////////////
struct AbsTime {
  std::string timeString;
  double msFraction;
};

////////////////////////////////////////////////////////////////////////////////
static AbsTime getAbsTime(const SystemClock::time_point& now) {
  auto const nowTimet = SystemClock::to_time_t(now);

  // time_t is in seconds, and we want ms resolution. Convert back and extract the ms.
  auto const nowFromTimet = SystemClock::from_time_t(nowTimet);
  auto const msDelta = std::chrono::duration_cast<std::chrono::milliseconds>(now - nowFromTimet);
  auto const msFraction = double(msDelta.count()) / 1000.0;

  std::stringstream timeStr;

  // Note - intentionally omitting timezone since it doesn't change. The app will always log
  // an initial entry that includes a reference time with the timezone.
  // SCW: Also note, using %Z on Windows is emitting "Pacific Standard Time" rather than "PST"
  const auto lt = sw::localtime(&nowTimet);
  timeStr << std::put_time(&lt, "%c");

  return AbsTime{timeStr.str(), msFraction};
}

}  // namespace log_detail

////////////////////////////////////////////////////////////////////////////////
inline void ConsoleFileLogHandler::onLog(SystemTimepoint logTime, Logger::Category category,
                                         const StringWrapper& msg, bool force) {
  // Determine what we'll log and exit early if nothing to do.
  bool const logToConsole = (_config.console_destination != ConsoleDestination::None) &&
                            Logger::canLogCategory(category, _config.consoleCategoryMask, force);
  bool const logToFile = _fout.is_open() && Logger::canLogCategory(category, _config.fileCategoryMask, force);
  if (!logToConsole && !logToFile) {
    return;
  }

  // Pre-compute abs time string if needed
  const auto& absTime = (logToConsole && _config.consoleTimeStyle == LoggerTimeStyle::Absolute) ||
                                (logToFile && _config.fileTimeStyle == LoggerTimeStyle::Absolute) ?
                            log_detail::getAbsTime(logTime) :
                            log_detail::AbsTime{std::string{}, 0};

  // Handle different log styles
  const auto& makeLogString = [&](LoggerTimeStyle timeStyle) -> std::string {
    switch (timeStyle) {
    case LoggerTimeStyle::Delta: {
      auto const& elapsed = logTime - _startTime;
      auto const& elapsedSecs = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed);
      return fmt::format("{:.3f}:{:4}: {}", elapsedSecs.count(), Logger::categoryCode(category), msg.c_str());
    }
    case LoggerTimeStyle::Absolute: {
      return fmt::format("{}.{:.3f}:{:4}: {}", absTime.timeString, absTime.msFraction,
                         Logger::categoryCode(category), msg.c_str());
    }
    case LoggerTimeStyle::None:
    default: {
      return fmt::format("{:4}: {}", Logger::categoryCode(category), msg.c_str());
    }
    }
  };

  const auto& consoleStr = logToConsole ? makeLogString(_config.consoleTimeStyle) : std::string{};
  const auto& fileStr = logToFile ? makeLogString(_config.fileTimeStyle) : std::string{};

  // Locking just around the actual output means messages can print out-of-order
  MutexLock lock(_lock);
  if (logToConsole) {
    auto& dest = (_config.console_destination == ConsoleDestination::Stderr) ? std::cerr : std::cout;
    dest << consoleStr << std::endl;
  }
  if (logToFile) {
    _fout << fileStr << system::ThisSystemTraits::newline() << std::flush;
  }
}

}  // namespace sw
