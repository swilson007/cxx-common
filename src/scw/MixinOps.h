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

namespace scw {

////////////////////////////////////////////////////////////////////////////////
/// Type defines 'bool equals(const T& that) const'
template <typename T>
struct EqualityMixin {
  friend bool operator==(const T& lhs, const T& rhs) { return lhs.equals(rhs); }
  friend bool operator!=(const T& lhs, const T& rhs) { return !(lhs.equals(rhs)); }
};

////////////////////////////////////////////////////////////////////////////////
/// Type defines only 'bool lessThan(const T& that) const'
template <typename T>
struct LessThanEqualityMixin {
  friend bool operator==(const T& lhs, const T& rhs) {
    return !lhs.lessThan(rhs) && !rhs.lessThan(lhs);
  }
  friend bool operator!=(const T& lhs, const T& rhs) {
    return lhs.lessThan(rhs) || rhs.lessThan(lhs);
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Class defines only 'lessThan(const T& that) const'
template <typename T>
struct CompareMixin {
  friend bool operator<(const T& lhs, const T& rhs) { return lhs.lessThan(rhs); }
  friend bool operator<=(const T& lhs, const T& rhs) {
    return lhs.lessThan(rhs) || !(rhs.lessThan(lhs));
  }
  friend bool operator>(const T& lhs, const T& rhs) { return rhs.lessThan(lhs); }
  friend bool operator>=(const T& lhs, const T& rhs) {
    return rhs.lessThan(lhs) || !(lhs.lessThan(rhs));
  }
};

}  // namespace scw
