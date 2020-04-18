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

#include "fixed_width_int_literals.h"
#include "misc.h"
#include "types.h"

#include <cstring>
#include <new>
#include <type_traits>

SW_NAMESPACE_BEGIN

using namespace ::sw::intliterals;

// clang-format off
/// These structs are just to support enable_if constructs
namespace detail {

template <class T>
struct NeedsConstruction :
  public std::integral_constant<bool, !std::is_arithmetic<T>::value>
{
};
template <class T>
struct NeedsDestruction :
  public std::integral_constant<bool, std::is_destructible<T>::value && !std::is_fundamental<T>::value>
{
};
template <class T>
struct IsMemCopyable :
  public std::integral_constant<bool, std::is_trivially_copyable<T>::value>
{
};

}
// clang-format on

using namespace ::sw::detail;

////////////////////////////////////////////////////////////////////////////////
/// This class contains move/copy/construct/destruction functions that are useful for
/// a container class like vector. For the purpose of performance, most of the functions here have
/// two variants that are matched by enable_if. The variants are for types that are safe to memcpy
/// (trivially copyable) or not, items that need actual destruction or not, items that have actual
/// constructors or not, etc.
///
template <typename T>
class MoveCopyOps {
  enum class Destruction { Unused };
  enum class NoDestruction { Unused };
  enum class Construction { Unused };
  enum class NoConstruction { Unused };
  enum class MemCopyable { Unused };
  enum class NotMemCopyable { Unused };

public:
  ////////////////////////////////////////////////////////////////////////////////
  /// Destruct count items starting at dest. Items are destructed in forward order.
  /// TODO: Reverse order?
  template <typename U = T,
            typename std::enable_if_t<NeedsDestruction<U>::value, Destruction> = Destruction::Unused>
  inline static void destructItems(T* dest, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      (&dest[i])->~T();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Destruct count items starting at dest. Items are destructed in forward order.
  template <typename U = T,
            typename std::enable_if_t<!NeedsDestruction<U>::value, NoDestruction> = NoDestruction::Unused>
  inline static void destructItems(T* dest, sizex count) {
    /// These type of items require no destruction!
    (void)dest;
    (void)count;
    nop();
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<NeedsConstruction<U>::value, Construction> = Construction::Unused>
  inline static void constructItemsFromItem(T* dest, sizex count, const T& item) {
    for (sizex i = 0; i < count; ++i) {
      auto memAddr = &(dest[i]);
      new (memAddr) T(item);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<!NeedsConstruction<U>::value, NoConstruction> = NoConstruction::Unused>
  inline static void constructItemsFromItem(T* dest, sizex count, const T& item) {
    /// These type of items don't need constructor calls
    std::fill(dest, dest + count, item);
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<NeedsConstruction<U>::value, Construction> = Construction::Unused>
  inline static void constructDefaultItems(T* dest, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      auto memAddr = &(dest[i]);
      new (memAddr) T();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<!NeedsConstruction<U>::value, NoConstruction> = NoConstruction::Unused>
  inline static void constructDefaultItems(T* dest, sizex count) {
    /// These type of items are safely set to 0 on construction
    std::memset(dest, 0, count * sizeof(T));
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<!IsMemCopyable<U>::value, NotMemCopyable> = NotMemCopyable::Unused>
  inline static void moveAssignItems(T* dest, const T* source, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      dest[i] = std::move(source[i]);  // move assign
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<IsMemCopyable<U>::value, MemCopyable> = MemCopyable::Unused>
  inline static void moveAssignItems(T* dest, const T* source, sizex count) {
    /// These types of items can safely use memcpy
    std::memcpy(dest, source, count * sizeof(T));
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<!IsMemCopyable<U>::value, NotMemCopyable> = NotMemCopyable::Unused>
  inline static void copyAssignItems(T* dest, const T* source, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      dest[i] = source[i];  // copy assign
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<IsMemCopyable<U>::value, MemCopyable> = MemCopyable::Unused>
  inline static void copyAssignItems(T* dest, const T* source, sizex count) {
    /// These types of items can safely use memcpy
    std::memcpy(dest, source, count * sizeof(T));
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<!IsMemCopyable<U>::value, NotMemCopyable> = NotMemCopyable::Unused>
  inline static void copyConstructItems(T* dest, const T* source, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      new (&dest[i]) T(source[i]);  // Copy-ctor
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<IsMemCopyable<U>::value, MemCopyable> = MemCopyable::Unused>
  inline static void copyConstructItems(T* dest, const T* source, sizex count) {
    /// These types of items can safely use memcpy
    std::memcpy(dest, source, count * sizeof(T));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Move (or copy) 'count' non-trivial items from 'source' to 'dest'
  template <typename U = T,
            typename std::enable_if_t<!IsMemCopyable<U>::value, NotMemCopyable> = NotMemCopyable::Unused>
  inline static void moveConstructItems(T* dest, const T* source, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      new (&dest[i]) T(std::move(source[i]));  // Move-ctor
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<IsMemCopyable<U>::value, MemCopyable> = MemCopyable::Unused>
  inline static void moveConstructItems(T* dest, const T* source, sizex count) {
    /// These types of items can safely use memcpy for moving
    std::memcpy(dest, source, count * sizeof(T));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Move (or copy) 'count' non-trivial items from 'source' to 'dest', and delete
  /// each item from 'source'
  template <typename U = T,
            typename std::enable_if_t<!IsMemCopyable<U>::value, NotMemCopyable> = NotMemCopyable::Unused>
  inline static void moveConstructAndDeleteItems(T* dest, const T* source, sizex count) {
    for (sizex i = 0; i < count; ++i) {
      // Move it from source to dest
      new (&dest[i]) T(std::move(source[i]));  // Move-ctor

      // And delete the source item now that it's been moved
      (&source[i])->~T();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  template <typename U = T,
            typename std::enable_if_t<IsMemCopyable<U>::value, MemCopyable> = MemCopyable::Unused>
  inline static void moveConstructAndDeleteItems(T* dest, const T* source, sizex count) {
    /// These types of items can safely use memcpy for moving
    std::memcpy(dest, source, count * sizeof(T));
  }
};

SW_NAMESPACE_END
