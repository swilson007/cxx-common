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
#include "fixed_width_int_literals.h"
#include "reallocator.h"
#include "types.h"

#include <memory>

namespace sw {

using namespace intliterals;

/// This could be enable_if'd to pick the right allocator (see Reallocator.h)
template <typename ReallocatorType = MallocReallocator<byte>>
class UniqueBufferType;
using UniqueBuffer = UniqueBufferType<>;

////////////////////////////////////////////////////////////////////////////////
template <typename ReallocatorType = MallocReallocator<byte>>
UniqueBufferType<ReallocatorType> makeUniqueBuffer(sizex size) {
  return UniqueBufferType<ReallocatorType>(ReallocatorType().allocate(size), size);
}

////////////////////////////////////////////////////////////////////////////////
// Wraps a memory buffer and assumes memory ownership. For now, this just
// assumes the standard allocator. Make it a template to support a different allocator
// if needed.
//
// Usage of this object is designed to have the same semantics as unique_ptr.
//
template <typename ReallocatorType>
class UniqueBufferType {
public:
  UniqueBufferType() noexcept {}

  /// The passed buffer must be from the same allocator or it's undefined.
  UniqueBufferType(byte* buffer, sizex size) noexcept : data_(buffer), size_(size) {}

  UniqueBufferType(UniqueBufferType& that) = delete;
  UniqueBufferType& operator=(UniqueBufferType& that) = delete;

  /// Proper noexcept move
  UniqueBufferType(UniqueBufferType&& that) noexcept :
      data_(std::exchange(that.data_, nullptr)),
      size_(std::exchange(that.size_, 0)) {}

  /// Proper Noexcept move assign
  UniqueBufferType& operator=(UniqueBufferType&& that) noexcept {
    if (this != &that) {
      if (data_) {
        allocator().deallocate(data_, size_);
      }

      data_ = std::exchange(that.data_, nullptr);
      size_ = std::exchange(that.size_, 0);
    }

    return *this;
  }

  /// Destroy this buffer. This will deallocate the underlying buffer memory
  ~UniqueBufferType() noexcept {
    if (data_) {
      allocator().deallocate(data_, size_);
    }
  }

  const byte* data() const noexcept { return data_; }
  byte* data() noexcept { return data_; }
  byte* cdata() noexcept { return data_; }
  sizex size() const noexcept { return size_; }
  bool empty() const noexcept { return data_ == nullptr; }
  byte& operator[](sizex i) { return data_[i]; }
  const byte& operator[](sizex i) const { return data_[i]; }
  explicit operator bool() const noexcept { return data_ != nullptr; }

  /// Extract the buffer, relinquishing ownership
  byte* release() noexcept {
    size_ = 0;
    return std::exchange(data_, nullptr);
  }

  /// Clear the buffer. This will deallocate if it's currently set
  void reset() noexcept {
    if (data_) {
      allocator().deallocate(data_, size_);
      data_ = nullptr;
      size_ = 0;
    }
  }

  void swap(UniqueBufferType& that) noexcept {
    std::swap(data_, that.data_);
    std::swap(size_, that.size_);
  }

  friend void swap(UniqueBufferType& lhs, UniqueBufferType& rhs) { lhs.swap(rhs); }

  // Allows comparison to nullptr
  friend bool operator==(const UniqueBufferType& x, std::nullptr_t) noexcept { return x.empty(); }
  friend bool operator==(std::nullptr_t, const UniqueBufferType& x) noexcept { return x.empty(); }
  friend bool operator!=(const UniqueBufferType& x, std::nullptr_t) noexcept { return !x.empty(); }
  friend bool operator!=(std::nullptr_t, const UniqueBufferType& x) noexcept { return !x.empty(); }

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// For now until we have an allocator member (via compressed pair)... this can
  /// then change to return a reallocator&
  ReallocatorType allocator() { return ReallocatorType(); }

  byte* data_ = nullptr;
  sizex size_ = 0u;
};

////////////////////////////////////////////////////////////////////////////////
/// Wraps a memory buffer and assumes *no* memory ownership.  The buffer is allowed
/// to be null.
class BufferView {
public:
  BufferView() = default;
  BufferView(const BufferView& that) = default;
  BufferView& operator=(const BufferView& that) = default;
  BufferView(BufferView&& that) noexcept = default;
  BufferView& operator=(BufferView&& that) noexcept = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates the BufferView from the buffer. If the buffer is valid (not null), then the
  /// the size must >= 1. ie, it's invalid (and undefined) to give a non-null buffer with
  /// size==0
  BufferView(byte* data, sizex size) noexcept : data_(data), size_(size) {
    SW_ASSERT((data_ == nullptr && size_ == 0) || (data_ != nullptr && size_ > 0));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates a buffer-view of the given unique buffer
  template <typename ReallocatorType = MallocReallocator<byte>>
  explicit BufferView(UniqueBufferType<ReallocatorType>& ub) noexcept : BufferView(ub.data(), ub.size()) {}

  /// A valid buffer must be non-null and have >0 size
  bool isValid() const noexcept { return data_ != nullptr && size_ > 0; }

  const byte* data() const noexcept { return data_; }
  byte* data() noexcept { return data_; }
  sizex size() const noexcept { return size_; }
  bool empty() const noexcept { return data_ == nullptr; }
  explicit operator bool() const noexcept { return data_ != nullptr; }
  byte& operator[](sizex i) { return data_[i]; }
  const byte& operator[](sizex i) const { return data_[i]; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Adjust the size bytes represented by the buffer. Note this in no way changes the
  /// actual buffer, just the size that a caller will associate with the buffer
  /// Changing the size to greater than the actual sized is undefined.
  void resize(sizex size) noexcept { size_ = size; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Clear the buffer. The resultant buffer will be null and size will be 0.
  void reset() noexcept {
    data_ = nullptr;
    size_ = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Advance the buffer by the number of bytes. The underlying buffer is *not* changed.
  BufferView& operator+=(sizex numBytes) {
    data_ += numBytes;
    size_ -= numBytes;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  void swap(BufferView& that) noexcept {
    std::swap(data_, that.data_);
    std::swap(size_, that.size_);
  }

  ////////////////////////////////////////////////////////////////////////////////
  friend void swap(BufferView& lhs, BufferView& rhs) { lhs.swap(rhs); }

  /// Allow comparison to nullptr
  friend bool operator==(const BufferView& x, std::nullptr_t) noexcept { return x.empty(); }
  friend bool operator==(std::nullptr_t, const BufferView& x) noexcept { return x.empty(); }
  friend bool operator!=(const BufferView& x, std::nullptr_t) noexcept { return !x.empty(); }
  friend bool operator!=(std::nullptr_t, const BufferView& x) noexcept { return !x.empty(); }

private:
  byte* data_ = nullptr;
  sizex size_ = 0_z;
};

////////////////////////////////////////////////////////////////////////////////
/// Wraps a memory buffer and assumes *no* memory ownership.  The buffer is allowed
/// to be null.
class ConstBufferView {
public:
  ConstBufferView() = default;
  ConstBufferView(ConstBufferView& that) = default;
  ConstBufferView& operator=(const ConstBufferView& that) = default;
  ConstBufferView(ConstBufferView&& that) noexcept = default;
  ConstBufferView& operator=(ConstBufferView&& that) noexcept = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates the BufferView from the buffer. If the buffer is valid (not null), then the
  /// the size must >= 1. ie, it's invalid (and undefined) to give a non-null buffer with
  /// size==0
  ConstBufferView(const byte* data, sizex size) noexcept : data_(data), size_(size) {
    SW_ASSERT((data_ == nullptr && size_ == 0) || (data_ != nullptr && size_ > 0));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates a buffer-view of the given unique buffer
  explicit ConstBufferView(const UniqueBuffer& ub) noexcept :
      ConstBufferView(ub.data(), ub.size()) {}

  /// A valid buffer must be non-null and have >0 size
  bool isValid() const noexcept { return data_ != nullptr && size_ > 0; }

  const byte* data() const noexcept { return data_; }
  sizex size() const noexcept { return size_; }
  bool empty() const noexcept { return data_ == nullptr; }
  explicit operator bool() const noexcept { return data_ != nullptr; }
  const byte& operator[](sizex i) const { return data_[i]; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Adjust the size bytes represented by the buffer. Note this in no way changes the
  /// actual buffer, just the size that a caller will associate with the buffer
  /// Changing the size to greater than the actual sized is undefined.
  void resize(sizex size) noexcept { size_ = size; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Clear the buffer. The resultant buffer will be null and size will be 0.
  void reset() noexcept {
    data_ = nullptr;
    size_ = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Advance the buffer by the number of bytes. The underlying buffer is *not* changed.
  ConstBufferView& operator+=(sizex numBytes) {
    data_ += numBytes;
    size_ -= numBytes;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  void swap(ConstBufferView& that) noexcept {
    std::swap(data_, that.data_);
    std::swap(size_, that.size_);
  }

  ////////////////////////////////////////////////////////////////////////////////
  friend void swap(ConstBufferView& lhs, ConstBufferView& rhs) { lhs.swap(rhs); }

  /// Allow comparison to nullptr
  friend bool operator==(const ConstBufferView& x, std::nullptr_t) noexcept { return x.empty(); }
  friend bool operator==(std::nullptr_t, const ConstBufferView& x) noexcept { return x.empty(); }
  friend bool operator!=(const ConstBufferView& x, std::nullptr_t) noexcept { return !x.empty(); }
  friend bool operator!=(std::nullptr_t, const ConstBufferView& x) noexcept { return !x.empty(); }

private:
  const byte* data_ = nullptr;
  sizex size_ = 0_z;
};

}  // namespace sw
