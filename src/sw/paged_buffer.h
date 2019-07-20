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
#include "types.h"

#include <vector>
#include <memory>
#include <cstring>

namespace sw {

////////////////////////////////////////////////////////////////////////////////
/// Buffer that can grow without reallocating by using memory pages
/// Only basic functions for now
template <sizex kPageSize = 1024, bool kZeroizeNewPages = false>
class PagedBuffer {
public:
  explicit PagedBuffer(sizex capacity = 0) {
    ensureCapacity(capacity);
    validateInvariants();
  }

  ////////////////////////////////////////////////////////////////////////////////
  sizex capacity() const { return pages_.size() * kPageSize; }

  ////////////////////////////////////////////////////////////////////////////////
  void reserve(sizex newCapacity) {
    ensureCapacity(newCapacity);

    validateInvariants();
  }

  ////////////////////////////////////////////////////////////////////////////////
  sizex size() const { return size_; }

  ////////////////////////////////////////////////////////////////////////////////
  sizex resize(sizex newSize) {
    ensureCapacity(newSize);
    size_ = newSize;

    validateInvariants();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Access single byte
  byte& operator[](sizex i) {
    const auto page = i / kPageSize;
    const auto pageByte = i % kPageSize;
    return pages_[page][pageByte];
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Access single byte
  const byte& operator[](sizex i) const {
    const auto page = i / kPageSize;
    const auto pageByte = i % kPageSize;
    return pages_[page][pageByte];
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Copies 'count' bytes from the source buffer onto the end of the paged array,
  /// growing the total array size by 'count'
  void append(const byte* source, sizex count) { return copyInto(size_, source, count); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Copies 'count' bytes from the source buffer into this buffer starting
  /// 'position'. This buffer will expand if needed.
  void copyInto(sizex position, const byte* source, sizex count) {
    ensureCapacity(position + count);
    const byte* end = source + count;

    // Copy into the first page
    auto pageNum = position / kPageSize;
    {
      const auto pageByte = position % kPageSize;
      const auto bytesLeftInPage = kPageSize - pageByte;
      auto& page = pages_[pageNum];
      const auto bytesToCopy = std::min(count, bytesLeftInPage);
      std::memcpy(&page[pageByte], source, bytesToCopy);
      source += bytesToCopy;
    }

    // Now copy into the remaining pages
    while (source < end) {
      auto& page = pages_[++pageNum];
      const auto bytesLeft = static_cast<sizex>(end - source);
      const auto bytesToCopy = std::min(bytesLeft, kPageSize);
      std::memcpy(&page[0], source, bytesToCopy);
      source += bytesToCopy;
    }

    // Finally update the size if we've grown
    if (position + count > size_)
      size_ = position + count;

    validateInvariants();
  }

  ////////////////////////////////////////////////////////////////////////////////
  void copyFrom(sizex position, byte* dest, sizex count) {
    const byte* end = dest + count;

    // Copy from the first page
    auto pageNum = position / kPageSize;
    {
      const auto pageByte = position % kPageSize;
      const auto bytesLeftInPage = kPageSize - pageByte;
      const auto& page = pages_[pageNum];
      const auto bytesToCopy = std::min(count, bytesLeftInPage);
      std::memcpy(dest, &page[pageByte], bytesToCopy);
      dest += bytesToCopy;
    }

    // Now read into the remaining pages
    while (dest < end) {
      auto& page = pages_[++pageNum];
      const auto bytesLeft = static_cast<sizex>(end - dest);
      const auto bytesToCopy = std::min(bytesLeft, kPageSize);
      std::memcpy(dest, &page[0], bytesToCopy);
      dest += bytesToCopy;
    }
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  void ensureCapacity(sizex newCapacity) {
    if (newCapacity <= capacity()) {
      return;
    }

    const auto newPageCount = newCapacity / kPageSize + (((newCapacity % kPageSize) == 0) ? 0 : 1);
    const auto oldPageCount = pages_.size();
    SW_ASSERT(newPageCount >= oldPageCount);
    for (sizex i = oldPageCount; i < newPageCount; ++i) {
      pages_.emplace_back(std::make_unique<byte[]>(kPageSize));
      if (kZeroizeNewPages) {
        std::memset(pages_.back().get(), 0, kPageSize);
      }
    }

    validateInvariants();
  }

private:
  void validateInvariants() { SW_ASSERT(pages_.size() * kPageSize >= size_); }

private:
  using Page = std::unique_ptr<byte[]>;
  std::vector<Page> pages_;
  sizex size_ = 0;  ///< size in bytes
};

}  // namespace sw
