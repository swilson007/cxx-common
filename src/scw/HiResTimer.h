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
#pragma once

#include <chrono>

namespace scw {

////////////////////////////////////////////////////////////////////////////////
/// Class for hi-resolution timing.
///
/// General usage:
/// <code>
/// HiResTimer hrt;
///  ... Do some stuff ...
/// auto elapsedTime = hrt.elapsed();
/// </code>
///
class HiResTimer {
public:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creation of the timer starts it
  HiResTimer() : start_(Clock::now()) {}

  ////////////////////////////////////////////////////////////////////////////////
  // @return Returns the duration since the timer was started
  std::chrono::microseconds elapsed() const { return elapsedDuration<std::chrono::microseconds>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// @return Returns the duration since the timer was started
  std::chrono::milliseconds elapsedMs() const {
    return elapsedDuration<std::chrono::milliseconds>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @return Returns the duration since the timer was started as floating point seconds
  std::chrono::duration<double> elapsedSecs() const {
    return elapsedDuration<std::chrono::duration<double>>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the duration, and restarts the timer
  ///
  /// @return Returns the duration since the timer was started
  std::chrono::microseconds update() {
    auto result = elapsed();
    start_ = Clock::now();
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the duration, and restarts the timer
  ///
  /// @return Returns the duration since the timer was started
  std::chrono::milliseconds updateMs() {
    auto result = elapsedMs();
    start_ = Clock::now();
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Restarts the timer
  void restart() { start_ = Clock::now(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Chrono helper that converts to floating point seconds.
  static double toSeconds(std::chrono::milliseconds v) { return v.count() / 1000.0; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Chrono helper that converts to floating point seconds.
  static double toSeconds(std::chrono::microseconds v) { return v.count() / 1000000.0; }

public:
  template <typename T>
  T elapsedDuration() const {
    T result = std::chrono::duration_cast<T>(Clock::now() - start_);
    return result;
  }

private:
  TimePoint start_;
};

}  // namespace scw
