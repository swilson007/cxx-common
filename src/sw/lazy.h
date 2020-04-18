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

#include "assert.h"

#include <functional>

////////////////////////////////////////////////////////////////////////////////
/// Contains useful miscellaneous functions
////////////////////////////////////////////////////////////////////////////////

SW_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
/// Wraps a POD value that is lazy-evaluated (once) and consumes no extra space. This
/// requires the value has an "invalid value" that will determine if it has been
/// evaluated or not. It also requires the 'get()' methods include a set function
/// which can be inconvienient of course. Use `LazyValue` or `LazyLambdaValue` otherwise.
template <typename T, T kInvalidValue>
class LazyPodValue {
public:
  explicit LazyPodValue(T const& value = kInvalidValue) noexcept : value_(value) {}

  LazyPodValue(LazyPodValue const&) = default;
  LazyPodValue& operator=(LazyPodValue const&) = default;
  LazyPodValue(LazyPodValue&&) noexcept = default;
  LazyPodValue& operator=(LazyPodValue&&) noexcept = default;
  ~LazyPodValue() = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// If the value is kInvalidValue, then the value will first be set to the return
  /// value of initFunc() then returned.  Otherwise, the value is returned.
  template <typename InitFunc>
  inline T& get(InitFunc const& initFunc) {
    return doGet(initFunc);
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename InitFunc>
  inline T const& get(InitFunc const& initFunc) const {
    return doGet(initFunc);
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  template <typename InitFunc>
  inline T& doGet(InitFunc const& initFunc) const {
    if (value_ == kInvalidValue) {
      value_ = initFunc();
      SW_ASSERT(value_ != kInvalidValue);  // Don't ever set the value to invalid value
    }
    return value_;
  }

  // Allow const semantics
  mutable T value_;
};

////////////////////////////////////////////////////////////////////////////////
/// Wraps a value that is lazy-evaluated once. This will consume extra space for
/// a boolean flag and the evaluation function. This is *not* thread-safe.
///
/// This version doesn't use std::function internally and may thus be more
/// performant than LazyValue, but at the expense of more challenging usage/syntax.
///
template <typename T, typename EvalFunc>
class LazyLambdaValue {
public:
  LazyLambdaValue() = default;
  explicit LazyLambdaValue(EvalFunc f) noexcept : evalFunc_(f) {}

  LazyLambdaValue(LazyLambdaValue const&) = default;
  LazyLambdaValue& operator=(LazyLambdaValue const&) = default;
  LazyLambdaValue(LazyLambdaValue&&) = default;
  LazyLambdaValue& operator=(LazyLambdaValue&&) = default;
  ~LazyLambdaValue() = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Gets the value. Lazy evaluates if needed
  T& get() { return doGet(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Gets the value. Lazy evaluates if needed
  T const& get() const { return doGet(); }

private:
  ////////////////////////////////////////////////////////////////////////////////
  T& doGet() const {
    if (!isSet_) {
      value_ = evalFunc_();
      isSet_ = true;
    }
    return value_;
  }

  // Allow const semantics
  mutable T value_;
  mutable bool isSet_ = {false};
  EvalFunc evalFunc_;
};

////////////////////////////////////////////////////////////////////////////////
/// Wraps a value that is lazy-evaluated once. This will consume extra space for
/// a boolean flag and the evaluation function. This is *not* thread-safe.
///
/// This version uses std::function and may have slightly worse performance than
/// LazyLambdaValue, but has easier to use syntax.
template <typename T>
class LazyValue : public LazyLambdaValue<T, std::function<T()>> {
  using BaseClass = LazyLambdaValue<T, std::function<T()>>;

public:
  explicit LazyValue(std::function<T()> f) noexcept : BaseClass(f) {}
};

/// TODO
template <typename T, typename Func>
LazyLambdaValue<T, Func> makeLazyValue(Func&& f) {
  return LazyLambdaValue<T, Func>(std::forward<Func>(f));
}
}
;  // namespace sw
