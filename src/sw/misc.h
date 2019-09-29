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

#include "assert.h"
#include "mixin_ops.h"
#include "types.h"

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

namespace sw {

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
      SW_ASSERT(false);  // We've recursed!
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

  ScopeGuard(ScopeGuard&& that) : func_(that.func_) { execute_ = std::exchange(that.execute_, false); }

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
      SW_ASSERT(mValue != kInvalidValue);  // Don't set the value to the invalid value ever (I hope)
    }
    return mValue;
  }

  // Allow const style semantics via mutable
  mutable T mValue;
};

////////////////////////////////////////////////////////////////////////////////
/// Wrapper template to make POD types into distinct types.
/// Includes invalid support and unset support, but you can just ignore what you don't need.
///
/// Example: Declare as follows:
///   class MyUint32 : public PodWrapperBase<u32> { using PodWrapperBase::PodWrapperBase; };
///
/// Note that doing a `using MyType = PodWrapperBase<...>` may not define a distinct class - all template
/// args must be different for it to be unique. Thus, you either subclass it, or use the
/// subclassing macro defined below: SW_DEFINE_POD_TYPE(MyUint32, u32, 0, 0)
///
template <typename T, T kUnsetValue = 0, T kInvalidValue = 0>
class PodWrapperBase {
public:
  using ValueType = T;

  static constexpr T unsetValue() { return kUnsetValue; }
  static constexpr T invalidValue() { return kInvalidValue; }

  constexpr PodWrapperBase() = default;

  /// Purposefully non-explicit conversion for ease of use
  constexpr PodWrapperBase(const T& v) : value(v) {}  // NOLINT

  explicit operator bool() const = delete;  ///> Prevent pain and suffering

  explicit operator T&() { return value; }
  constexpr explicit operator const T&() const { return value; }

  void set(const T& v) { value = v; }
  T& get() { return value; }
  const T& get() const { return value; }

  bool isInvalid() const { return value == kInvalidValue; }
  bool isValid() const { return value != kInvalidValue; }
  bool isSet() const { return value != kUnsetValue; }
  bool isUnset() const { return value == kUnsetValue; }

  friend std::ostream& operator<<(std::ostream& outs, const PodWrapperBase& u) { return outs << u.value; }

  struct Hasher {
    size_t operator()(const PodWrapperBase& pw) const { return std::hash<T>()(pw.value); }
  };

  // Allow direct public access
  T value = kUnsetValue;
};

////////////////////////////////////////////////////////////////////////////////
/// Defines a distinct class that wraps a POD value. The class will include full
/// comparison and equality operators, and increment/decrement operators
/// ex: `SW_DEFINE_POD_TYPE(MyPodType, u32, 0, ~uint32(0));`
#define SW_DEFINE_POD_TYPE(classname_, podType_, unsetValue_, invalidValue_)   \
  struct classname_ :                                                          \
      public ::sw::PodWrapperBase<podType_, unsetValue_, invalidValue_>,       \
      public ::sw::EqualityMixin<classname_>,                                  \
      public ::sw::CompareMixin<classname_> {                                  \
    classname_() = default;                                                    \
    constexpr classname_(podType_ v) : PodWrapperBase(v) {}                    \
    classname_(const classname_& key) = default;                               \
    classname_& operator=(const classname_& key) = default;                    \
    static classname_ unset() { return classname_(unsetValue()); }             \
    static classname_ invalid() { return classname_(invalidValue()); }         \
    classname_& operator++() {                                                 \
      ++value;                                                                 \
      return *this;                                                            \
    }                                                                          \
    classname_ operator++(int) {                                               \
      auto tmp = *this;                                                        \
      ++value;                                                                 \
      return tmp;                                                              \
    }                                                                          \
    classname_& operator--() {                                                 \
      --value;                                                                 \
      return *this;                                                            \
    }                                                                          \
    classname_ operator--(int) {                                               \
      auto tmp = *this;                                                        \
      --value;                                                                 \
      return tmp;                                                              \
    }                                                                          \
    bool equals(const classname_& that) const { return value == that.value; }  \
    bool lessThan(const classname_& that) const { return value < that.value; } \
  };                                                                           \
  static_assert(sizeof(classname_) == sizeof(podType_), "bad size")

////////////////////////////////////////////////////////////////////////////////
/// Operators for bitfields. Generally best to declare this just after the enum,
/// or just after the class decl that contains an enum.
///
/// The asPod() function is just less typing than the static_cast approach to get
/// the enum out as the underlying integral (POD) type
#define SW_DEFINE_ENUM_BITFIELD_OPERATORS(T)                                              \
  static_assert(std::is_unsigned<std::underlying_type<T>::type>::value,                   \
                "Bitmask enums must use unsigned types");                                 \
  constexpr std::underlying_type<T>::type asPod(T enumValue) {                            \
    return static_cast<std::underlying_type<T>::type>(enumValue);                         \
  }                                                                                       \
  constexpr T operator~(T v) { return static_cast<T>(~asPod(v)); }                        \
  constexpr T operator|(T lhs, T rhs) { return static_cast<T>(asPod(lhs) | asPod(rhs)); } \
  constexpr T operator&(T lhs, T rhs) { return static_cast<T>(asPod(lhs) & asPod(rhs)); } \
  constexpr T operator^(T lhs, T rhs) { return static_cast<T>(asPod(lhs) ^ asPod(rhs)); } \
  inline T& operator|=(T& lhs, T rhs) {                                                   \
    lhs = static_cast<T>(asPod(lhs) | asPod(rhs));                                        \
    return lhs;                                                                           \
  }                                                                                       \
  inline T& operator&=(T& lhs, T rhs) {                                                   \
    lhs = static_cast<T>(asPod(lhs) & asPod(rhs));                                        \
    return lhs;                                                                           \
  }                                                                                       \
  inline T& operator^=(T& lhs, T rhs) {                                                   \
    lhs = static_cast<T>(asPod(lhs) ^ asPod(rhs));                                        \
    return lhs;                                                                           \
  }                                                                                       \
  static_assert(true, "Put a semicolon after the macro definition!")

};  // namespace sw
