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
/// To add something to the cache, use put `cache.put(key, value)`. I chose
/// *not* to use the name `insert()` in similar vein to std::unordered_map because
/// the behavior is a little different. `put` will update an existing value while `insert`
/// won't.
///
/// There's a number of ways to extract items from the cache.
/// * Use `operator[]`. This behaves just like std::unordered_map in that it will access the
///   cache item if it exists, or create a default value item if it doesn't exist.
///    cache[1] = "foo";
///    std::cout << cache[1] << std::endl;
/// * Use `get()` which copies out an item into an existing variable. Not perfect, but won't
///   auto-create a default value if the cache item doesn't exist.
///    std:string str;
///    if (cache.get(1, str))
///        std::cout << str << std::endl;
/// * Use `find()` which behaves similar to `unordered_map::find()`. It will return an
///   `UnorderedIterator` which equals `end()` if the item isn't in the cache.
///    auto iter = cache.find(1);
///    if (iter != cache.end())
///        std::cout << *iter << std::endl;
///
/// Ordered vs Unordered iterators. Why have both? The primary purpose for the unordered
/// version is to support `find()` and `erase()`. The unordered keeps a direct pointer
/// into the hashmap, while the ordered does not. It would be possible to just have the
/// ordered version, but accessing the hashmap value from it would then require an extra
/// key lookup. My anticipation for using/needing the ordered version is low, like probably
/// just for debugging, thus I'm keeping the unordered as the primary iterator.
///
/// SCW: Basically just writing this for fun... haven't done one before.
////////////////////////////////////////////////////////////////////////////////
template <typename Key, typename T, bool kAutoPurge = true>
class LruCache {
  using KeyValuePair = std::pair<Key, T>;
  using List = std::list<KeyValuePair>;
  using ListIter = typename List::iterator;
  using ListReverseIter = typename List::reverse_iterator;
  using Map = SysHashMap<Key, ListIter>;
  using MapIter = typename Map::iterator;
  using ConstMapIter = typename Map::const_iterator;

public:
  class UnorderedIterator;
  class OrderedIterator;
  using iterator = UnorderedIterator;
  using Iterator = UnorderedIterator;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates the cache and sets the maximum size before items get purged.
  LruCache(sizex maxSize = 10) : maxSize_(maxSize < 1 ? 1 : maxSize) {}

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
  /// Purge any entries such that the cache will be within it's max-size. Not necessary
  /// when auto-purge is enabled
  void purge() {
    doPurge();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Clear the cache completely
  void clear() {
    list_.clear();
    map_.clear();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Does the cache contain an entry for the given key
  bool contains(const Key& key) const {
    return map_.find(key) != map_.end();
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
  /// Remove a cached value using an iterator
  sizex erase(const iterator& iter) {
    if (iter == end()) {
      return 0;
    }

    auto mapIter = iter.iter_;
    auto listIter = mapIter->second;
    list_.erase(listIter);
    map_.erase(mapIter);
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
      list_.push_front(std::make_pair(key, T{}));
      auto listIter = list_.begin();
      map_.insert(std::make_pair(key, listIter));
      doAutoPurge();
      return getValue(listIter);
    }

    // The item already exists - bring it to the front of the list since it's been "used"
    onValueUsed(iter);
    return getValue(iter);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Inserts or updates mapped value associated with the given key
  void put(const Key& key, const T& value) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      // New item
      list_.push_front(std::make_pair(key, value));
      map_.insert(std::make_pair(key, list_.begin()));
      doAutoPurge();
    } else {
      // Item already exists. Update the mapped value and push it to the front
      getValue(iter) = value;
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Inserts or updates mapped value associated with the given key
  void put(const Key& key, T&& value) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      // New item
      list_.push_front(std::make_pair(key, std::move(value)));
      map_.insert(std::make_pair(key, list_.begin()));
      doAutoPurge();
    } else {
      // Item already exists. Update the mapped value and push it to the front
      getValue(iter) = std::move(value);
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @value Will be a copy of the value if it exists, otherwise it is unchanged
  /// @return true if the value was found
  bool get(const Key& key, T& value) {
    MapIter iter = map_.find(key);
    if (iter == map_.end()) {
      return false;
    }

    onValueUsed(iter);
    value = getValue(iter);
    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Find the cache element with the specified key. If it doesn't exist, endUnordered() is
  /// returned. If it does exist, it will be refreshed to the front of the cache, and
  /// and UnorderedIterator to it is returned.
  iterator find(const Key& key) {
    MapIter iter = map_.find(key);
    if (iter != map_.end()) {
      onValueUsed(iter);
    }

    return UnorderedIterator(std::move(iter));
  }

  ////////////////////////////////////////////////////////////////////////////////
  iterator begin() noexcept { return UnorderedIterator(map_.begin()); }
  iterator end() noexcept { return UnorderedIterator(map_.end()); }

  /// Ordered iteration. I suspect this is primarily for debugging, so it's not the
  /// primary begin/end. ie. you'll have to use a full for loop. Also, the ordered
  /// iterator only contains the cache'd value, and not the key.
  OrderedIterator beginOrdered() noexcept { return OrderedIterator(list_.begin()); }
  OrderedIterator endOrdered() noexcept { return OrderedIterator(list_.end()); }

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

    const Key& key() const { return LruCache::getKey(iter_); }
    T& value() { return LruCache::getValue(iter_); }
    const T& value() const { return LruCache::getValue(iter_); }

    T& operator*() { return LruCache::getValue(iter_); }
    const T& operator*() const { return LruCache::getValue(iter_); }

    friend bool operator==(const UnorderedIterator& i1, const UnorderedIterator& i2) {
      return i1.iter_ == i2.iter_;
    }
    friend bool operator!=(const UnorderedIterator& i1, const UnorderedIterator& i2) {
      return i1.iter_ != i2.iter_;
    }

  private:
    explicit UnorderedIterator(MapIter&& iter) noexcept : iter_(std::move(iter)) {}
    MapIter iter_;

    friend class LruCache;
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

    const Key& key() const { return LruCache::getKey(iter_); }
    T& value() { return LruCache::getValue(iter_); }
    const T& value() const { return LruCache::getValue(iter_); }

    T& operator*() { return LruCache::getValue(iter_); }
    const T& operator*() const { return LruCache::getValue(iter_); }

    friend bool operator==(const OrderedIterator& i1, const OrderedIterator& i2) {
      return i1.iter_ == i2.iter_;
    }
    friend bool operator!=(const OrderedIterator& i1, const OrderedIterator& i2) {
      return i1.iter_ != i2.iter_;
    }

  private:
    explicit OrderedIterator(ListIter&& iter) noexcept : iter_(std::move(iter)) {}
    ListIter iter_;

    friend class LruCache;
  };

private:
  static const Key& getKey(const ListIter& listIter) { return (*listIter).first; }
  static const Key& getKey(const ListReverseIter& listIter) { return (*listIter).first; }
  static T& getValue(const ListIter& listIter) { return (*listIter).second; }
  static const Key& getKey(const MapIter& mapIter) { return mapIter->first; }
  static T& getValue(const MapIter& mapIter) { return getValue(mapIter->second); }

  ////////////////////////////////////////////////////////////////////////////////
  void onValueUsed(const ConstMapIter& iter) {
    list_.splice(list_.begin(), list_, iter->second);
  }

  ////////////////////////////////////////////////////////////////////////////////
  void doPurge() {
    // Check the size so we can avoid creating any iterators for the nothing to purge case
    if (size() <= maxSize_)
      return;

    // Start at the back and delete things towards the front until we're within our
    // desired size. Using a reverse iterator
    auto listIter = list_.rbegin();
    do {
      auto mapIter = map_.find(LruCache::getKey(listIter));
      SW_ASSERT(mapIter != map_.end());
      map_.erase(mapIter);

      // Remove the list entry, doing the reverse-iterator junk
      listIter = std::make_reverse_iterator(list_.erase(std::next(listIter).base()));
    } while (size() > maxSize_);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Will auto purge the cache if configured. Should optimize to a NOP when auto
  /// purge is disabled
  void doAutoPurge() {
    if (kAutoPurge) {
      doPurge();
    }
  }

private:
  /// The map will hold list iterators. Note that list iterators don't invalidate
  /// unless you erase the underlying item. They're effectively like having a node pointer
  SysHashMap<Key, ListIter> map_;
  List list_;
  sizex maxSize_ = 10;
};

}  // namespace sw
