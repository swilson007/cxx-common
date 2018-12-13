#pragma once

#include "Assert.h"
#include "FixedWidthIntLiterals.h"
#include "Reallocator.h"
#include "Types.h"

#include <memory>

namespace scw {

using namespace intliterals;

template <typename ReallocatorType = MallocReallocator<byte>>
class UniqueBufferType;

using UniqueBuffer = UniqueBufferType<>;

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
  static UniqueBufferType create(sizex size) {
    return UniqueBufferType(static_cast<byte*>(std::malloc(size)), size);
  }

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
    return data_;
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
  BufferView(BufferView& that) = default;
  BufferView& operator=(const BufferView& that) = default;
  BufferView(BufferView&& that) noexcept = default;
  BufferView& operator=(BufferView&& that) noexcept = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates the BufferView from the buffer. If the buffer is valid (not null), then the
  /// the size must >= 1. ie, it's invalid (and undefined) to give a non-null buffer with
  /// size==0
  BufferView(byte* data, sizex size) noexcept : data_(data), size_(size) {
    Assert((data_ == nullptr && size_ == 0) || (data_ != nullptr && size_ > 0));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates a buffer-view of the given unique buffer
  BufferView(UniqueBuffer& ub) noexcept : BufferView(ub.data(), ub.size()) {}

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
}  // namespace scw
