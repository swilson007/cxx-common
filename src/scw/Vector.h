#pragma once

#include "Assert.h"
#include "Misc.h"
#include "Reallocator.h"
#include "Types.h"

#if UNTIL_BUFFER
#  include "UniqueBuffer.h"
#endif

#include <type_traits>
#include <vector>

namespace scw {

template <typename T, typename Reallocator>
class VectorBase;

// The vector type.
template <typename T, typename Reallocator =
                          std::conditional_t<std::is_trivially_destructible<T>::value &&
                                               std::is_trivially_copyable<T>::value,
                                           MallocReallocator<T>, ReallocatorAdapter<T, std::allocator<T>>>>
using Vector = VectorBase<T, Reallocator>;

////////////////////////////////////////////////////////////////////////////////
/// A vector with a bit more flexibility/efficiency than std::vector for POD types.
/// - Uses a "Reallocator". For now, assumes the reallocator is stateless
/// - Supports capacity setting on construction
/// - *Not* specialized for 'bool' space efficiency, so use std::vector for that
/// - Can consume a std::unique_ptr<T> for no-copy semantics
/// - Conforms to the std::vector interface where possible.
/// - data() returns nullptr for empty vector
///
/// I'm using the stateless allocator until I get a compressed-pair class in there.
////////////////////////////////////////////////////////////////////////////////
template <typename T, typename Reallocator>
class VectorBase {
public:
  using value_type = T;
  using size_type = ::std::size_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reallocator = Reallocator;
  using move_copy_ops = MoveCopyOps<T>;

  ////////////////////////////////////////////////////////////////////////////////
  ~VectorBase() noexcept {
    move_copy_ops::destructItems(begin_, size());
    allocator().deallocate(begin_, capacity());
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase() noexcept : begin_(nullptr), end_(nullptr), capacity_(nullptr) {}

  ////////////////////////////////////////////////////////////////////////////////
  /// Copy a vector
  VectorBase(const VectorBase& that) :
      begin_(allocator().allocate(that.capacity())),
      end_(begin_ + that.size()),
      capacity_(begin_ + that.capacity()) {
    move_copy_ops::copyConstructItems(begin_, that.begin_, that.size());
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Move a vector
  VectorBase(VectorBase&& that) noexcept :
      begin_(std::exchange(that.begin_, nullptr)),
      end_(std::exchange(that.end_, nullptr)),
      capacity_(std::exchange(that.capacity_, nullptr)) {
    nop();
  }

  ////////////////////////////////////////////////////////////////////////////////
  explicit VectorBase(size_type count) :
      begin_(allocator().allocate(count)),
      end_(begin_ + count),
      capacity_(end_) {
    move_copy_ops::constructDefaultItems(begin_, count);
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase(size_type count, size_type capacity) :
      begin_(allocator().allocate(capacity)),
      end_(begin_ + count),
      capacity_(begin_ + capacity) {
    move_copy_ops::constructDefaultItems(begin_, count);
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase(size_type count, const value_type& value) :
      begin_(allocator().allocate(count)),
      end_(begin_ + count),
      capacity_(end_) {
    move_copy_ops::constructItemsFromItem(begin_, count, value);
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase(size_type count, size_type capacity, const value_type& value) :
      begin_(allocator().allocate(capacity)),
      end_(begin_ + count),
      capacity_(begin_ + capacity) {
    move_copy_ops::constructItemsFromItem(begin_, count, value);
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase(std::initializer_list<T> init) :
      begin_(allocator().allocate(init.size())),
      end_(begin_ + init.size()),
      capacity_(end_) {
    T* slot = begin_;
    for (const auto& value : init) {
      move_copy_ops::constructItemsFromItem(slot, 1, value);
      ++slot;
    }
  }

#if UNTIL_BUFFER
  ////////////////////////////////////////////////////////////////////////////////
  // Assume ownership
  explicit VectorBase(UniqueBuffer&& buffer) {
    auto bufferSize = buffer.size();
    auto count = bufferSize / sizeof(T);
    begin_ = buffer.release().release();
    end_ = begin_ + count;
    capacity_ = begin_ + count;
  }
#endif

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase& operator=(const VectorBase& that) {
    if (this != &that) {
      begin_ = allocator().allocate(that.capacity());
      end_ = begin_ + that.size();
      capacity_ = begin_ + that.capacity();
      move_copy_ops::copyConstructItems(begin_, that.begin_, that.size());
    }
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  VectorBase& operator=(VectorBase&& that) noexcept {
    if (this != &that) {
      begin_ = std::exchange(that.begin_, nullptr);
      end_ = std::exchange(that.end_, nullptr);
      capacity_ = std::exchange(that.capacity_, nullptr);
    }
    return *this;
  }

  iterator begin() noexcept { return begin_; }
  const_iterator begin() const noexcept { return begin_; }
  const_iterator cbegin() const noexcept { return begin_; }
  iterator end() noexcept { return end_; }
  const_iterator end() const noexcept { return end_; }
  const_iterator cend() const noexcept { return end_; }

  ////////////////////////////////////////////////////////////////////////////////
  void assign(size_type count, const T& value) {
    reserve(count);
    move_copy_ops::constructItemsFromItem(begin_, count, value);

    // If we need to shrink, then use resize for correct item destruction
    if (size() > count) {
      resize(count);
    } else {
      end_ = begin_ + count;
    }
  }

  reference at(size_type pos) { return begin_[pos]; }
  const_reference at(size_type pos) const { return begin_[pos]; }
  value_type& front() { return *begin_; }
  const_reference front() const { return *begin_; }
  reference back() { return *(end_ - 1); }
  const_reference back() const { return *(end_ - 1); }

  ////////////////////////////////////////////////////////////////////////////////
  reference operator[](size_type pos) { return begin_[pos]; }
  const_reference operator[](size_type pos) const { return begin_[pos]; }

  ////////////////////////////////////////////////////////////////////////////////
  T* data() noexcept { return begin_; }
  const T* data() const noexcept { return begin_; }

  ////////////////////////////////////////////////////////////////////////////////
  inline size_type size() const noexcept {
    auto result = static_cast<size_type>(end_ - begin_);
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  inline bool empty() const { return begin_ == end_; }

  ////////////////////////////////////////////////////////////////////////////////
  void reserve(size_type newCapacity) {
    // Per std::vector, no shrinking allowed
    if (newCapacity < capacity()) {
      return;
    }

    auto oldSize = size();
    begin_ = allocator().reallocate(begin_, oldSize, capacity(), newCapacity);
    end_ = begin_ + oldSize;
    bufferEnd() = begin_ + newCapacity;
  }

  ////////////////////////////////////////////////////////////////////////////////
  inline size_type capacity() const {
    auto result = static_cast<size_type>(bufferEnd() - begin_);
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Since clear needs to delete items, not sure how it can safely be noexcept
  void clear() { resize(0); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Since resize needs to delete items, not sure how it can safely be noexcept
  void resize(size_type count) {
    auto oldSize = size();
    if (count < oldSize) {
      shrinkTo(count);
    } else {
      reserve(count);
      auto itemsToAdd = count - oldSize;
      move_copy_ops::constructDefaultItems(&begin_[oldSize], itemsToAdd);
      end_ = begin_ + count;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  void resize(size_type count, const value_type& value) {
    auto oldSize = size();
    if (count < oldSize) {
      shrinkTo(count);
    } else {
      reserve(count);
      auto itemsToAdd = count - oldSize;
      move_copy_ops::constructItemsFromItem(&begin_[oldSize], itemsToAdd, value);
      end_ = begin_ + count;
    }
  }

  void push_back(const T& value) {
    autoGrow();
    move_copy_ops::copyConstructItems(end_, &value, 1);
    ++end_;
  }
  void push_back(T&& value) {
    autoGrow();
    move_copy_ops::moveConstructItems(end_, &value, 1);
    ++end_;
  }

  void pop_back() {
    --end_;
    move_copy_ops::destructItems(*end_, 1);
  }

  // TODO
#if 0
    template <class... Args>
    reference emplace_back(Args&& ... args);

    template <class InputIt>
    void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<T> ilist);
    size_type max_size() const noexcept;
#endif

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// For now until we have an allocator member (via compressed pair)... this can
  /// then change to return a reallocator&
  reallocator allocator() { return reallocator(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Call when the size is shrinking. Will destruct items
  void shrinkTo(size_type newCount) {
    Assert(newCount < size());
    //        inline static void destructItems(T* dest, size_type count) {
    auto itemsToDestruct = size() - newCount;
    auto firstItem = size() - itemsToDestruct;
    move_copy_ops::destructItems(&begin_[firstItem], itemsToDestruct);
    end_ = begin_ + newCount;
  }

  inline T*& bufferEnd() { return capacity_; }
  inline T* const& bufferEnd() const { return capacity_; }

  /// For now, to grow we'll just double our size. TODO: Think about this
  inline void autoGrow() {
    if (end_ == bufferEnd()) {
      reserve(capacity() * 2);
    }
  }

private:
  T* begin_;
  T* end_;
  T* capacity_;
};

#if 0
/// The beginnings of an stl::allocator supporting version of the vector. At this time, I'm not planning to finish it.
/// For an allocator based vector, use std::vector

////////////////////////////////////////////////////////////////////////////////
/// A vector with a bit more flexibility/efficiency than std::vector for POD types.
/// - Does not support an allocator.
/// - Supports capacity setting on construction
/// - *Not* specialized for 'bool' space efficiency, so use std::vector for that
/// - Can consume a std::unique_ptr<T> for no-copy semantics
/// - Conforms to the std::vector interface where possible.
/// - data() returns nullptr for empty vector
////////////////////////////////////////////////////////////////////////////////
template <typename T, class Allocator = std::allocator<T>, sizex kCapacityAlignmentSize = 16>
class VectorBase {
public:
    using value_type = T;
    using size_type = size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using allocator_type = Allocator;

    ////////////////////////////////////////////////////////////////////////////////
    ~VectorBase() {
        if (begin_ != nullptr) {
            allocator().deallocate(begin_, capacity());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    VectorBase() noexcept (std::is_nothrow_default_constructible<allocator_type>::value) :
#  if COMPRESSED_PAIR
        begin_(nullptr), end_(nullptr), capAllocPair_(nullptr) {}
#  else
        begin_(nullptr), end_(nullptr), capacity_(nullptr) {}
#  endif

    explicit VectorBase(const Allocator& allocator);

    VectorBase(const VectorBase& that);
    VectorBase(const VectorBase& that, const Allocator& allocator);
    VectorBase(VectorBase&& that) noexcept;
    VectorBase(VectorBase&& that, const Allocator& allocator) noexcept;

    explicit VectorBase(size_type count, const Allocator& allocator = Allocator());
    VectorBase(size_type count, size_type capacity, const Allocator& = Allocator());
    VectorBase(size_type count, const value_type& value, const Allocator& = Allocator());
    VectorBase(size_type count, const value_type& value, size_type capacity, const Allocator& = Allocator());

    explicit VectorBase(std::initializer_list<T> init, const Allocator& allocator = Allocator());
    VectorBase(std::initializer_list<T> init, size_type capacity, const Allocator& = Allocator());

    // Assume ownership
    explicit VectorBase(UniqueBuffer buffer, const Allocator& = Allocator());
    VectorBase(std::unique_ptr<T> buffer, size_type bufferSize, const Allocator& = Allocator());

    VectorBase& operator=(const VectorBase& that);
    VectorBase& operator=(VectorBase&& that) noexcept;

    void assign(size_type count, const T& value);
    template <class InputIt>
    void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<T> ilist);

    reference at(size_type pos);
    const_reference at(size_type pos) const;
    reference front(size_type pos);
    const_reference front(size_type pos) const;
    reference back(size_type pos);
    const_reference back(size_type pos) const;

    reference operator[](size_type pos) { return begin_[pos]; }
    const_reference operator[](size_type pos) const { return begin_[pos]; }

    T* data() noexcept { return begin_; }
    const T* data() const noexcept { return begin_; }

    inline size_type size() const noexcept {
        auto result = static_cast<size_type>(end_ - begin_);
        return result;
    }

    size_type max_size() const noexcept;
    inline bool empty() const { return begin_ == end_; }

    template <typename U = T>
    typename std::enable_if<std::is_trivially_copyable<U>::value, void>::type
    inline copyBuffer(U* dest, U* source, size_type count) {
        std::memcpy(dest, source, count * sizeof(U));
    }

    template <typename U = T>
    typename std::enable_if<!std::is_trivially_copyable<U>::value, void>::type
    inline copyBuffer(U* dest, U* source, size_type count) {
        // Need to do one by one move
        T* oldBuffer = begin_;
        for (size_type i = 0; i < count; ++i) {
            dest[i] = std::move(source[i]);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    void reserve(size_type newCapacity) {
        // Per std::vector, no shrinking allowed
        if (newCapacity < capacity()) {
            return;
        }

        // Find the actual size to allocate
        auto & alloc = allocator();
        auto newBuffer = alloc.allocate(newCapacity * sizeof(T));
        auto oldSize = size();
        auto oldCapacity = capacity();
        Assert(oldCapacity == 0 ? (begin_ == nullptr) : (begin_ != nullptr));

        // Move items from old buffer to new buffer
        copyBuffer(newBuffer, begin_, oldSize);

        // Done with old buffer
        if (oldCapacity > 0) {
            alloc.deallocate(begin_, oldCapacity);
        }

        // Finally we can update our internal state
        begin_ = newBuffer;
        end_ = newBuffer + oldSize;
        bufferEnd() = newBuffer + newCapacity;
    }

    inline size_type capacity() const {
        auto result = static_cast<size_type>(bufferEnd() - begin_);
        return result;
    }

    void clear() noexcept;
    void resize(size_type count);
    void resize(size_type count, const value_type& value);

    void push_back(const T& value) {
        autoGrow();
        *end_ = value;
        ++end_;
    }
    void push_back(T&& value) {
        autoGrow();
        *end_ = std::move(value);
        ++end_;
    }
    void pop_back();

    template <class... Args>
    reference emplace_back(Args&& ... args);

private:
    /// For now, to grow we'll just double our size. TODO: Think about this
    inline void autoGrow() {
        if (end_ == bufferEnd()) {
            reserve(capacity() * 2);
        }
    }

private:
    T* begin_;
    T* end_;

#  if COMPRESSED_PAIR
    // This pair contains our buffer end an our allocator
    ::boost::compressed_pair<T*, Allocator> capAllocPair_;

    inline T*& bufferEnd() { return capAllocPair_.first(); }
    inline T* const& bufferEnd() const { return capAllocPair_.first(); }
    inline Allocator& allocator() { return capAllocPair_.second(); }
#  else
    T* capacity_;
    Allocator allocator_;
    inline T*& bufferEnd() { return capacity_; }
    inline T* const& bufferEnd() const { return capacity_; }
    inline Allocator& allocator() { return allocator_; }
#  endif
};
#endif

}  // namespace scw
