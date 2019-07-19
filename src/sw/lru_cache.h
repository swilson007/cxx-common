////////////////////////////////////////////////////////////////////////////////
/// Copyright 2019 Steven C. Wilson
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

#include <sw/assert.h>
#include <sw/types.h>

#if SW_USE_ROBIN_HASH_MAP
// TODO: Put this in the project
#  include <tsl/robin_map.h>
#else
#  include <unordered_map>
#endif

#include <list>
#include <memory>

namespace sw {

#if SW_USE_ROBIN_HASH_MAP
/// Using a much faster hash-map implementation then std::unordered map. But the downside
/// is the sizeof this map is larger than by around 32 bytes (72 vs 40)
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using SysHashMap = tsl::robin_map<Key, T, Hash, KeyEqual, Allocator>;
#else
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using SysHashMap = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;
#endif

////////////////////////////////////////////////////////////////////////////////
/// This will implement an LRU-cache.
///
/// The structure of the cache will be a doublely-linked list used in conjunction with hashmap.
/// Cache lookup will thus be O(1) via the hashmap, which will point to a node in the list.
/// The list nodes will maintain the ordering. When a cached item is pinged, it will move to
/// the front of the list (also O(1)). Items at the end of the list can get speculatively
/// purged.
///
/// Basically just writing this for fun... haven't done one before.
////////////////////////////////////////////////////////////////////////////////
template <typename Key, typename T>
class LruCache {
  using List = std::list<T>;
  using ListIter = typename List::iterator;
  using Map = SysHashMap<Key, ListIter>;
  using MapIter = typename Map::iterator;
  using ConstMapIter = typename Map::const_iterator;

public:
  class UnorderedIterator;
  class OrderedIterator;

  ////////////////////////////////////////////////////////////////////////////////
  /// Return the number of items in the map
  sizex size() const noexcept { return map_.size(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Is the map empty?
  bool empty() const noexcept { return map_.empty(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Check if the given key is mapped to a value in the cache
  bool exists(const Key& key) const noexcept {
    auto iter = map_.find(key);
    return iter != map_.end();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Refreshes a cache item if it exists, causing it to move to the front
  void refresh(const Key& key) noexcept {
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Remove a cached value
  sizex erase(const Key& key) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      return 0;
    }

    // Remove the node from the list and the map
    list_.erase(iter->second);
    map_.erase(iter);
    return 1;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Extract the cached value for the given key. If the value isn't in the cache, a
  /// default constructed value will be created, placed in the cache, and returned.
  T& operator[](const Key& key) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      // No item. We needs to create a default value in that case
      // First insert a default value to the front of the list, then add it to the map
      list_.push_front(T());
      auto listIter = list_.begin();
      map_.insert(std::make_pair(key, listIter));
      return *listIter;
    }

    // The item already exists - bring it to the front of the list since it's been "used"
    onValueUsed(iter);
    return *iter->second;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Inserts or updates mapped value associated with the given key
  void put(const Key& key, const T& value) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      // New item
      list_.push_front(value);
      map_.insert(std::make_pair(key, list_.begin()));
    } else {
      // Item already exists. Update the mapped value and push it to the front
      *iter->second = value;
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Inserts or updates mapped value associated with the given key
  void put(const Key& key, T&& value) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      // New item
      list_.push_front(std::move(value));
      map_.insert(std::make_pair(key, list_.begin()));
    } else {
      // Item already exists. Update the mapped value and push it to the front
      auto listIter = iter->second;
      *iter->second = std::move(value);
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @value Will be a copy of the value if it exists, otherwise it is unchanged
  /// @return true if the value was found
  bool get(const Key& key, T& value) {
    ConstMapIter iter = map_.find(key);
    if (iter == map_.end()) {
      return false;
    }

    onValueUsed(iter);
    value = *iter->second;
    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Find the cache element with the specified key. If it doesn't exist, endUnordered() is
  /// returned. If it does exist, it will be refreshed to the front of the cache, and
  /// and UnorderedIterator to it is returned.
  UnorderedIterator find(const Key& key) {
    MapIter iter = map_.find(key);
    if (iter != map_.end()) {
      onValueUsed(iter);
    }

    return UnorderedIterator(std::move(iter));
  }

  ////////////////////////////////////////////////////////////////////////////////
  UnorderedIterator beginUnordered() noexcept { return UnorderedIterator(map_.begin()); }
  UnorderedIterator endUnordered() noexcept { return UnorderedIterator(map_.end()); }
  OrderedIterator begin() noexcept { return OrderedIterator(list_.begin()); }
  OrderedIterator end() noexcept { return OrderedIterator(list_.end()); }

  // TODO: purge functions

  ////////////////////////////////////////////////////////////////////////////////
  /// Accessing an element using the unordered iterator won't change the LRU ordering
  class UnorderedIterator {
  public:
    UnorderedIterator& operator++() {
      ++iter_;
      return *this;
    }

    UnorderedIterator operator++(int) {
      UnorderedIterator tmp = *this;
      operator++();
      return tmp;
    }

    UnorderedIterator& operator--() {
      --iter_;
      return *this;
    }

    UnorderedIterator operator--(int) {
      UnorderedIterator tmp = *this;
      operator--();
      return tmp;
    }

    const Key& key() const { return iter_->first; }
    T& value() { return *iter_->second; }
    const T& value() const { return *iter_->second; }

    T& operator*() { return *iter_->second; }
    const T& operator*() const { return *iter_->second; }

    friend bool operator==(const UnorderedIterator& i1, const UnorderedIterator& i2) {
      return i1.iter_ == i2.iter_;
    }
    friend bool operator!=(const UnorderedIterator& i1, const UnorderedIterator& i2) {
      return i1.iter_ != i2.iter_;
    }

  private:
    explicit UnorderedIterator(MapIter&& iter) noexcept : iter_(std::move(iter)) {}
    MapIter iter_;

    friend LruCache;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Accessing an element using the ordered iterator won't change the LRU ordering
  class OrderedIterator {
  public:
    OrderedIterator& operator++() {
      ++iter_;
      return *this;
    }

    OrderedIterator operator++(int) {
      OrderedIterator tmp = *this;
      operator++();
      return tmp;
    }

    OrderedIterator& operator--() {
      --iter_;
      return *this;
    }

    OrderedIterator operator--(int) {
      OrderedIterator tmp = *this;
      operator--();
      return tmp;
    }

    T& value() { return *iter_; }
    const T& value() const { return *iter_; }

    T& operator*() { return *iter_; }
    const T& operator*() const { return *iter_; }

    friend bool operator==(const OrderedIterator& i1, const OrderedIterator& i2) {
      return i1.iter_ == i2.iter_;
    }
    friend bool operator!=(const OrderedIterator& i1, const OrderedIterator& i2) {
      return i1.iter_ != i2.iter_;
    }

  private:
    explicit OrderedIterator(ListIter&& iter) noexcept : iter_(std::move(iter)) {}
    ListIter iter_;

    friend LruCache;
  };

private:
  void onValueUsed(const ConstMapIter& iter) {
    list_.splice(list_.begin(), list_, iter->second);
  }

private:
  /// The map will hold list iterators. Note that list iterators don't invalidate
  /// unless you erase the underlying item. They're effectively like having a node pointer
  SysHashMap<Key, ListIter> map_;
  List list_;
};

}  // namespace sw
