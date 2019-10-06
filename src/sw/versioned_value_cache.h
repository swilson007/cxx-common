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
#pragma once

#include "fixed_width_int_literals.h"
#include "types.h"

#include <vector>

namespace sw {

using namespace sw::intliterals;

////////////////////////////////////////////////////////////////////////////////
/// This class manages versioned copies of a specific value in a thread safe way.
/// Once a copy has been made of the source value, it will be cached with this
/// object as long as it version is the same as the source value. The version is
/// a simple counter and increments any time the value is changed. Checked out
/// copies will be auto-checked in upon destruction if they are still valid.
///
/// The underlying type must be movable and copyable, and for this class to be
/// useful, copies should be expensive and moves should be cheap.
///
/// The use case is fairly specific:
/// You need to use a value for some duration where the value is MT guarded, but
/// you don't (or can't) want to hold the lock for the long duration. So instead,
/// you copy the value and use the copy unlocked for the duration. Creating the
/// copy is expensive, so you'd like to cache the copy for use the next time.
/// And of course, the underlying value can change so you can't reliably just
/// give back the copy at the end.
///
/// See the unit test for example usage.
///
/// @tparam T The value type. Must be copyable and movable, and moves should be cheap.
template <typename T, typename Mutex, typename LockGuard>
class VersionedValueCache {
public:
  static constexpr u32 kInvalidVersion = ~0_u32;
  using ValueType = T;

  /// The Value class is handled out
  class Value {
  public:
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;

    // Movable
    Value(Value&& that) :
        _value(std::move(that._value)),
        _library(that._library),
        _version(std::exchange(that._version, kInvalidVersion)) {}

    // Movable
    Value& operator=(Value&& that) {
      _value = std::move(that._value);
      _library = that._library;
      _version = std::exchange(that._version, kInvalidVersion);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// The item will auto-checkin upon destruction if needed. This will acquire the lock.
    ~Value() {
      if (_library && _version != kInvalidVersion) {
        LockGuard lock(*(_library->_mutex));
        _library->checkin(Value(std::move(_value), _library, _version), lock);
      }
    }

    const T& value() const { return _value; }
    T& value() { return _value; }

  private:
    void invalidate() { _version = kInvalidVersion; }

    ////////////////////////////////////////////////////////////////////////////////
    Value(T&& value, VersionedValueCache* library, u32 version) :
        _value(std::move(value)),
        _library(library),
        _version(version) {}

    T _value;
    VersionedValueCache* _library = nullptr;
    u32 _version = kInvalidVersion;

    friend VersionedValueCache;
  };

  ~VersionedValueCache() {
    // Invalidate and clear our copies so they don't try to auto-checkin
    clearCopies();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Create a versioned value cache, giving it the mutex to use with accessing value copies
  VersionedValueCache(std::mutex& mutex) : _mutex(&mutex) {}

  ////////////////////////////////////////////////////////////////////////////////
  /// Sets the new main item. This will signify a new version and clear any cached copies
  /// Must lock the mutex
  void setValue(T&& item, sizex addCopyCount = 0) {
    LockGuard lock(*_mutex);
    setValue(std::move(item), lock, addCopyCount);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Sets the new main item with the lock already acquired.
  void setValue(T&& item, const LockGuard& /*locktag*/, sizex addCopyCount) {
    _value = std::move(item);

    // Increment the version which invalidates all copies
    ++_version;
    clearCopies();

    // Add copies if asked
    for (sizex i = 0; i < addCopyCount; ++i) {
      _copies.emplace_back(Value{T{_value}, this, _version});
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  Value checkout() {
    LockGuard lock(*_mutex);
    return checkout(lock);
  }

  ////////////////////////////////////////////////////////////////////////////////
  Value checkout(const LockGuard&) {
    // If we have a cached value, we can return it.
    if (!_copies.empty()) {
      auto result = std::move(_copies.back());
      _copies.pop_back();
      return result;
    }

    // Otherwise, make a new copy and return that
    return Value(T{_value}, this, _version);
  }

  ////////////////////////////////////////////////////////////////////////////////
  void checkin(Value&& item) {
    LockGuard lock(*_mutex);
    return checkin(std::move(item), lock);
  }

  ////////////////////////////////////////////////////////////////////////////////
  void checkin(Value&& item, const LockGuard&) {
    // Cache the item if it's version is still good. Otherwise let it die
    if (item._version == _version) {
      _copies.push_back(std::move(item));
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Return the number of cached copies. Primarily for debugging. Must lock the mutex
  sizex copyCount() const {
    LockGuard lock(*_mutex);
    return _copies.size();
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// Clear any copies but don't trigger the auto check-in
  void clearCopies() {
    for (auto& copy : _copies) {
      copy.invalidate();
    }
    _copies.clear();
  }

  /// The current value
  T _value;

  /// Current copies
  std::vector<Value> _copies;

  /// Mutex for checkouts
  Mutex* _mutex = nullptr;

  /// The current version
  std::atomic_uint32_t _version = {0};

  friend class Value;
};

template <typename T, typename Mutex, typename LockGuard>
constexpr u32 VersionedValueCache<T, Mutex, LockGuard>::kInvalidVersion;

}  // namespace sw
