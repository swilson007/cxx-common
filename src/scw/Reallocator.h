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
#include "FixedWidthIntLiterals.h"
#include "Misc.h"
#include "MoveCopyOps.h"
#include "Types.h"

#include <new>
#include <type_traits>

namespace scw {

using namespace ::scw::intliterals;

////////////////////////////////////////////////////////////////////////////////
/// Reallocator classes will use malloc/free to avoid unneeded object ctor calls
/// They have a semantic support for the 'realloc' notion, but only the trivial-type
/// version uses the actual realloc calls
///
/// Intended to map (loosely) to the std::allocator specs. Especially the newer versions
/// that don't include construct/destroy.
template <typename T>
class ReallocatorConcept {
public:
  ReallocatorConcept() = default;

  /// See std:allocator::allocate
  T* allocate(sizex count);

  /// See std::realloc for general idea.
  T* reallocate(T* oldAddr, sizex existingCount, sizex oldCount, sizex newCount);

  /// See std:allocator::deallocate
  void deallocate(T* addr, sizex) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
/// Reallocator classes will use malloc/free to avoid unneeded object ctor calls
/// They have a semantic support for the 'realloc' notion, but only the trivial-type
/// version uses the actual realloc calls
///
/// Intended to map (loosely) to the std::allocator specs. Especially the newer versions
/// that don't include construct/destroy.
template <typename T>
class MallocReallocator {
public:
  static_assert(std::is_trivially_copyable<T>::value,
                "MallocReallocator only works with trivially copyable types");
  static_assert(std::is_trivially_constructible<T>::value,
                "MallocReallocator only works with trivially constructable types");
  static_assert(std::is_trivially_destructible<T>::value,
                "MallocReallocator only works with trivially destructable types");

  using move_copy_ops = MoveCopyOps<T>;
  using value_type = T;

  ////////////////////////////////////////////////////////////////////////////////
  void deallocate(T* addr, sizex) noexcept { std::free(addr); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Allocate using malloc. Throws if there's a problem
  T* allocate(sizex count) {
    // Ensure no overflow
    if (count > (~0_z / sizeof(T))) {
      throw std::bad_alloc();
    }
    auto addr = static_cast<T*>(std::malloc(count * sizeof(T)));
    if (addr == nullptr) {
      throw std::bad_alloc();
    }

    return addr;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// This will return a new array that has 'existingCount' count items copied from
  /// the given oldAddr. The new buffer will have space for newCount items
  T* reallocate(T* oldAddr, sizex existingCount, sizex /*oldCount*/, sizex newCount) {
    auto newBuffer = static_cast<T*>(std::realloc(oldAddr, newCount * sizeof(T)));
    if (newBuffer == nullptr) {
      throw std::bad_alloc();
    }

    return newBuffer;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// This adapts a regular allocator to be a reallocator. The actual 'reallocate'
/// call will always allocate a new buffer however. That call will also take care
/// moving items from the old to the new buffer.
template <typename T, class Allocator = std::allocator<T>>
class ReallocatorAdapter {
public:
  using move_copy_ops = MoveCopyOps<T>;
  using allocator = Allocator;

  ////////////////////////////////////////////////////////////////////////////////
  /// Allocate using malloc. Throws if there's a problem
  T* allocate(sizex count) { return Allocator().allocate(count); }

  ////////////////////////////////////////////////////////////////////////////////
  void deallocate(T* addr, sizex count) noexcept { return Allocator().deallocate(addr, count); }

  ////////////////////////////////////////////////////////////////////////////////
  /// This will return a new array that has 'count' items copied from
  /// the given oldBuffer. The new buffer will have space for capacity items
  /// Note that this can't actually use a realloc, so it just allocates
  /// a new buffer and appropriately copies/moves items.
  /// @param oldCount Needs to be the count used during the previous allocate() or
  ///   reallocate().
  T* reallocate(T* oldAddr, sizex existingCount, sizex oldCount, sizex newCount) {
    // Move items from old buffer to new buffer
    auto newAddr = allocate(newCount);
    if (oldAddr != nullptr) {
      // Move construct items from the old to the new memory region
      // SCW: Consider: should maybe move-construct and delete as we go in case
      //  objects are large, or they are maintaining high-watermark internal counts
      move_copy_ops::moveConstructItems(newAddr, oldAddr, existingCount);

      // Delete items from the old to the new memory region
      move_copy_ops::destructItems(oldAddr, existingCount);

      deallocate(oldAddr, oldCount);
    }

    return newAddr;
  }
};

}  // namespace scw
