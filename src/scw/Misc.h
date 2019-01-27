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

#include "Assert.h"
#include "MixinOps.h"
#include "Types.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
/// Contains useful miscellaneous functions
////////////////////////////////////////////////////////////////////////////////

namespace scw {

////////////////////////////////////////////////////////////////////////////////
class NotImplementedError : public std::logic_error {
  using logic_error::logic_error;
};

////////////////////////////////////////////////////////////////////////////////
/// Convert a thread id to an integer
inline sizex toInteger(const decltype(std::this_thread::get_id())& threadId) {
  sizex result = std::hash<std::thread::id>{}(threadId);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
class EntryCounter {
public:
  int inc() { return ++count_; }
  void dec() { --count_; }

private:
  std::atomic_int count_;
};

////////////////////////////////////////////////////////////////////////////////
class NoRenentryGuard {
public:
  NoRenentryGuard(EntryCounter& counter) : counter_(counter) {
    if (counter_.inc() > 1) {
      Assert(false);  // We've recursed!
    }
  }
  ~NoRenentryGuard() { counter_.dec(); }

private:
  EntryCounter& counter_;
};

////////////////////////////////////////////////////////////////////////////////
/// Moveable scope-guard. Can't be copied. Won't execute function after being moved.
template <typename Func>
class ScopeGuard {
public:
  explicit ScopeGuard(Func&& f) : func_(f) {}
  ScopeGuard(const ScopeGuard& that) = delete;
  ScopeGuard& operator=(const ScopeGuard& that) = delete;

  ScopeGuard(ScopeGuard&& that) : func_(that.func_) {
    execute_ = std::exchange(that.execute_, false);
  }

  ScopeGuard& operator=(ScopeGuard&& that) {
    func_ = std::move(that.func_);
    execute_ = std::exchange(that.execute_, false);
    return *this;
  }

  ~ScopeGuard() {
    if (execute_) {
      func_();
    }
  }

private:
  bool execute_ = true;
  Func func_;
};

////////////////////////////////////////////////////////////////////////////////
/// Usage: auto scopeGuard1 = makeScopeGuard([](){ dosomething(); });
template <typename Func>
ScopeGuard<Func> makeScopeGuard(Func&& f) {
  return ScopeGuard<Func>(std::forward<Func>(f));
}

////////////////////////////////////////////////////////////////////////////////
/// Wraps a value that is lazy-evaluated (once) and consumes no extra space. This
/// requires the value has an "invalid value" that will determine if it has been
/// evaluated or not.
template <typename T, T kInvalidValue>
class LazyValue {
public:
  explicit LazyValue(const T& value = kInvalidValue) noexcept : mValue(value) {}

  LazyValue(const LazyValue&) = default;
  LazyValue& operator=(const LazyValue&) = default;
  LazyValue(LazyValue&&) noexcept = default;
  LazyValue& operator=(LazyValue&&) noexcept = default;
  ~LazyValue() = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// If the value is kInvalidValue, then the value will first be set to the return
  /// value of initFunc() then returned.  Otherwise, the value is returned.
  template <typename InitFunc>
  inline T& get(const InitFunc& initFunc) {
    return doGet(initFunc);
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename InitFunc>
  inline const T& get(const InitFunc& initFunc) const {
    return doGet(initFunc);
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  template <typename InitFunc>
  inline T& doGet(const InitFunc& initFunc) const {
    if (mValue == kInvalidValue) {
      mValue = initFunc();
      Assert(mValue != kInvalidValue);  // Don't set the value to the invalid value ever (I hope)
    }
    return mValue;
  }

  // Allow const style semantics via mutable
  mutable T mValue;
};

////////////////////////////////////////////////////////////////////////////////
/// Wrapper template to make POD types into distinct types.
/// Includes invalid support, but you can just ingore if you don't need invalid.
///
/// Example: Declare as follows:
///   class MyUint32 : public PodWrapper<u32> { using PodWrapper::PodWrapper; };
///
template <typename T, T kInvalidValue = 0>
class PodWrapper :
    public EqualityMixin<PodWrapper<T, kInvalidValue>>,
    public CompareMixin<PodWrapper<T, kInvalidValue>> {
public:
  using ValueType = T;
  static constexpr T kInvalid = kInvalidValue;

  PodWrapper() = default;

  // Explicit conversion intentional
  PodWrapper(const T& v) : value(v) {}

  // Non-Explicit intentionally
  operator T&() { return value; }
  operator const T&() const { return value; }

  void set(const T& v) { value = v; }
  T& get() { return value; }
  const T& get() const { return value; }
  bool isInvalid() const { return value == kInvalidValue; }

  // TODO Test these
  PodWrapper& operator++() {
    ++value;
    return *this;
  }
  PodWrapper operator++(int) {
    auto tmp = *this;
    ++value;
    return tmp;
  }

  PodWrapper& operator--() {
    --value;
    return *this;
  }
  PodWrapper operator--(int) {
    auto tmp = *this;
    --value;
    return tmp;
  }

  bool equals(const PodWrapper& that) const { return value == that.value; }
  bool lessThan(const PodWrapper& that) const { return value < that.value; }

  friend std::ostream& operator<<(std::ostream& outs, const PodWrapper& u) {
    return outs << u.value;
  }

  struct Hasher {
    size_t operator()(const PodWrapper& pw) const { return std::hash<T>()(pw.value); }
  };

  // Allow direct public access
  T value = kInvalidValue;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, T kInvalidValue>
constexpr T PodWrapper<T, kInvalidValue>::kInvalid;

////////////////////////////////////////////////////////////////////////////////
/// Operators for bitfields. Generally best to declare this just after the enum,
/// or just after the class decl that contains an enum.
///
/// asPod() function is just less typing than the static_cast approach to get the enum out as
/// the underlying integral (POD) type
#define SCW_DEFINE_ENUM_BITFIELD_OPERATORS(T)                                     \
  static_assert(std::is_unsigned<std::underlying_type<T>::type>::value,           \
                "Bitmask enums must use unsigned types");                         \
  std::underlying_type<T>::type asPod(T enumValue) {                              \
    using PodType = typename std::underlying_type<T>::type;                       \
    return static_cast<PodType>(enumValue);                                       \
  }                                                                               \
  T operator|(T lhs, T rhs) {                                                     \
    using PodType = typename std::underlying_type<T>::type;                       \
    return static_cast<T>(static_cast<PodType>(lhs) | static_cast<PodType>(rhs)); \
  }                                                                               \
  T operator&(T lhs, T rhs) {                                                     \
    using PodType = typename std::underlying_type<T>::type;                       \
    return static_cast<T>(static_cast<PodType>(lhs) & static_cast<PodType>(rhs)); \
  }                                                                               \
  T operator^(T lhs, T rhs) {                                                     \
    using PodType = typename std::underlying_type<T>::type;                       \
    return static_cast<T>(static_cast<PodType>(lhs) ^ static_cast<PodType>(rhs)); \
  }                                                                               \
  T& operator|=(T& lhs, T rhs) {                                                  \
    using PodType = typename std::underlying_type<T>::type;                       \
    lhs = static_cast<T>(static_cast<PodType>(lhs) | static_cast<PodType>(rhs));  \
    return lhs;                                                                   \
  }                                                                               \
  T& operator&=(T& lhs, T rhs) {                                                  \
    using PodType = typename std::underlying_type<T>::type;                       \
    lhs = static_cast<T>(static_cast<PodType>(lhs) & static_cast<PodType>(rhs));  \
    return lhs;                                                                   \
  }                                                                               \
  T& operator^=(T& lhs, T rhs) {                                                  \
    using PodType = typename std::underlying_type<T>::type;                       \
    lhs = static_cast<T>(static_cast<PodType>(lhs) ^ static_cast<PodType>(rhs));  \
    return lhs;                                                                   \
  }                                                                               \
  static_assert(true, "Put a semicolon after the macro definition!")

};  // namespace scw
