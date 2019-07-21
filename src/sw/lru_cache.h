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
#include <sw/fixed_width_int_literals.h>
#include <sw/types.h>

#if SW_USE_ROBIN_HASH_MAP
// TODO: Put this in the project
#  include <tsl/robin_map.h>
#else
#  include <unordered_map>
#endif

#include <algorithm>
#include <list>
#include <memory>

namespace sw {

using namespace sw::intliterals;

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

namespace lru_detail {
template <typename LruCache>
class UnorderedIterator;
template <typename LruCache>
class ConstUnorderedIterator;

template <typename LruCache>
class OrderedIterator;
template <typename LruCache>
class ConstOrderedIterator;
}  // namespace lru_detail

////////////////////////////////////////////////////////////////////////////////
/// # Overview
/// This will implement an LRU-cache.
///
/// The structure of the cache will be a doubly-linked list used in conjunction with hashmap.
/// Cache lookup will thus be O(1) via the hashmap, which will point to a node in the list.
/// The list nodes will maintain the ordering. When a cached item is pinged, it will move to
/// the front of the list (also O(1)). Items at the end of the list can get speculatively
/// purged.
///
/// The cache can be configured to auto-purge or not via a template parameter. The parameter
/// approach was chosen rather than a constructor argument simply for performance. ie. the
/// optimizer should remove any checks to auto-purge when it's is disabled.
///
/// # Insertion
/// * Use `operator[]` with similar semantics to std::unordered_map:
///     cache[1] = "foo";
/// * Use `put(key, value)`. This method was intenionally not named `insert()` because the
///   semanitics are a bit different to `unordered_map::insert`. Specifically, the `put` method
///   will overwrite an exisiting entry where `unordered_map::insert` will not.
///     cache.put(1, "foo");
///
/// # Retrieval
/// * Use `operator[]`. This behaves just like std::unordered_map in that it will access the
///   cached item if it exists, or create a default value item if it doesn't exist.
///    std::cout << cache[1] << std::endl;
/// * Use `get()` which copies out an item into an existing variable. Not perfect, but won't
///   auto-create a default value if the cache item doesn't exist.
///    std:string str;
///    if (cache.get(1, str))
///        std::cout << str << std::endl;
/// * Use `find()` which behaves similar to `unordered_map::find()`. It will return an
///  `Iterator` which equals `end()` if the item isn't cached.
///    auto iter = cache.find(1);
///    if (iter != cache.end())
///        std::cout << *iter << std::endl;
/// * Use `cfind()` to get back a ConstIterator. It will be `cend()` if the item isn't cached.
///    auto iter = cache.cfind(1);
///    if (iter != cache.cend())
///        std::cout << *iter << std::endl;
///
/// # Iterators
/// The iterators work for basic needs but they aren't fully compliant with `std`.
/// The appropriate iterator traits and types aren't set, std::make_reverse_iterator
/// isn't going to work.
/// It's all TODO.
///
/// Ordered vs Unordered iterators. Why have both? The primary purpose for the unordered
/// version is to support `find()` (and `erase()`) and not require a copy on retrieval or
/// or an auto-insert on retrieval. The unordered type  keeps a direct pointer into the
/// hashmap, while the ordered does not. It would be possible to just have the
/// ordered version, but accessing the hashmap value from the ordered would thus require
/// an extra key lookup. My anticipation for using/needing the ordered version is low,
/// like probably just for debugging, thus I'm keeping both with the unordered as the
/// primary iterator.
///
/// SCW: Basically just writing this for fun... haven't done one before.
////////////////////////////////////////////////////////////////////////////////
template <typename Key, typename T, bool kAutoPurge = true>
class LruCache {
  using KeyValuePair = std::pair<Key, T>;
  using ListType = std::list<KeyValuePair>;
  using ListIter = typename ListType::iterator;
  using MapType = SysHashMap<Key, ListIter>;
  using ConstListIter = typename ListType::const_iterator;
  using Map = SysHashMap<Key, ListIter>;
  using MapIter = typename Map::iterator;
  using ConstMapIter = typename Map::const_iterator;

  using ThisType = LruCache<Key, T, kAutoPurge>;

public:
  using UnorderedIterator = lru_detail::UnorderedIterator<ThisType>;
  using ConstUnorderedIterator = lru_detail::ConstUnorderedIterator<ThisType>;
  using OrderedIterator = lru_detail::OrderedIterator<ThisType>;
  using ConstOrderedIterator = lru_detail::ConstOrderedIterator<ThisType>;
  using Iterator = UnorderedIterator;
  using ConstIterator = ConstUnorderedIterator;
  using KeyType = Key;
  using ValueType = T;

  ////////////////////////////////////////////////////////////////////////////////
  /// Creates the cache and sets the maximum size before items get purged.
  explicit LruCache(sizex maxSizeValue = 10) : maxSize_(std::max(maxSizeValue, 1_z)) {}

  // Cached values will be destructed normally
  ~LruCache() = default;

  // Create a copy of the given cache
  LruCache(const LruCache& that) : maxSize_(that.maxSize_) { doEmptyCopyFrom(that); }

  // Copies the given cache to this cache. All prior entires in this cache are discarded
  LruCache& operator=(const LruCache& that) {
    if (this != &that) {
      clear();
      maxSize_ = that.maxSize_;
      doEmptyCopyFrom(that);
    }
    return *this;
  }

  // Moves the given cache to this. Not noexcept because list/map moves aren't noexcept
  LruCache(LruCache&&) = default;

  // Moves the given cache to this. Not noexcept because list/map moves aren't noexcept
  LruCache& operator=(LruCache&&) = default;

  ////////////////////////////////////////////////////////////////////////////////
  /// Return the number of items in the map
  sizex size() const noexcept { return map_.size(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Return the number maximum number of items allowed in the cache. If kAutoPurge is
  /// `false`, then this is a soft-limit. It can be exceeded, but a `purge()` call will
  /// remove items to achieve this size.
  sizex maxSize() const noexcept { return maxSize_; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Sets the maximum size of the map. Will purge items when `kAutoPurge` is
  /// true and the maximum size is reduced.
  void setMaxSize(sizex maxSizeValue) noexcept {
    auto newMaxSize = std::max(maxSizeValue, 1_z);
    bool isSmaller = newMaxSize < maxSize_;
    maxSize_ = newMaxSize;
    if (isSmaller)
      doAutoPurge();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Is the map empty?
  bool empty() const noexcept { return map_.empty(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Check if the given key is mapped to a value in the cache. Does *not* change
  /// cache ordering. Use `refresh()` for that case.
  bool contains(const KeyType& key) const { return map_.find(key) != map_.end(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Refreshes a cache item if it exists, causing it to move to the front
  void refresh(const KeyType& key) {
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      onValueUsed(iter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Purge any entries such that the cache will be within it's max-size. Not necessary
  /// when auto-purge is enabled
  void purge() { doPurge(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Clear the cache completely
  void clear() {
    list_.clear();
    map_.clear();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Remove a cached value
  sizex erase(const KeyType& key) {
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
  Iterator erase(const ConstIterator& iter) {
    if (iter == cend())
      return end();

    auto mapIter = iter.iter_;
    auto listIter = mapIter->second;
    list_.erase(listIter);
    return Iterator(map_.erase(mapIter));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Extract the cached value for the given key. If the value isn't in the cache, a
  /// default constructed value will be created, placed in the cache, and returned.
  T& operator[](const KeyType& key) {
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
  void put(const KeyType& key, const T& value) {
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
  void put(const KeyType& key, T&& value) {
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
  bool get(const KeyType& key, T& value) {
    MapIter iter = map_.find(key);
    if (iter == map_.end()) {
      return false;
    }

    onValueUsed(iter);
    value = getValue(iter);
    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Find the cache element with the specified key. If it doesn't exist, end() is
  /// returned. If it does exist, it will be refreshed to the front of the cache, and
  /// and a valid Iterator to it is returned.
  Iterator find(const KeyType& key) {
    MapIter iter = map_.find(key);
    if (iter != map_.end()) {
      onValueUsed(iter);
    }

    return Iterator(iter);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Find the cache element with the specified key. If it doesn't exist, cend() is
  /// returned. If it does exist, it will be refreshed to the front of the cache, and
  /// and a valid ConstIterator to it is returned.
  /// Note that we can't have a `find() const` since find needs to refresh it's cached
  /// value. Thus to get one with a const iterator there is `cfind`
  ConstIterator cfind(const KeyType& key) { return find(key); }

  ////////////////////////////////////////////////////////////////////////////////
  Iterator begin() noexcept { return Iterator(map_.begin()); }
  Iterator end() noexcept { return Iterator(map_.end()); }
  ConstIterator cbegin() const noexcept { return ConstIterator(map_.begin()); }
  ConstIterator cend() const noexcept { return ConstIterator(map_.end()); }

  /// Ordered iteration. I suspect this is primarily for debugging/testing, so it's
  /// not the primary begin/end. ie. you'll have to use a full for loop.
  OrderedIterator beginOrdered() noexcept { return OrderedIterator(list_.begin()); }
  OrderedIterator endOrdered() noexcept { return OrderedIterator(list_.end()); }
  ConstOrderedIterator beginOrdered() const noexcept { return ConstOrderedIterator(list_.cbegin()); }
  ConstOrderedIterator endOrdered() const noexcept { return ConstOrderedIterator(list_.cend()); }
  ConstOrderedIterator cbeginOrdered() const noexcept { return ConstOrderedIterator(list_.cbegin()); }
  ConstOrderedIterator cendOrdered() const noexcept { return ConstOrderedIterator(list_.cend()); }

private:
  // Clarity Helpers
  static const KeyType& getKey(const ConstListIter& listIter) noexcept { return (*listIter).first; }
  static const T& getValue(const ConstListIter& listIter) noexcept { return (*listIter).second; }
  static T& getValue(const ListIter& listIter) noexcept { return (*listIter).second; }
  static const KeyType& getKey(const ConstMapIter& mapIter) noexcept { return mapIter->first; }
  static T& getValue(const ConstMapIter& mapIter) noexcept { return getValue(mapIter->second); }
  static T& getValue(const MapIter& mapIter) noexcept { return getValue(mapIter->second); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Call this when a cached value is used, thus pushing it to the front of the list.
  void onValueUsed(const ConstMapIter& iter) { list_.splice(list_.begin(), list_, iter->second); }

  ////////////////////////////////////////////////////////////////////////////////
  void doPurge() {
    // Delete the last (aka oldest) item till our size is ok
    while (size() > maxSize_) {
      auto listIter = --list_.end();
      auto mapIter = map_.find(LruCache::getKey(listIter));
      SW_ASSERT(mapIter != map_.end());
      map_.erase(mapIter);
      list_.erase(listIter);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Will auto purge the cache if configured. Should optimize to a NOP when auto
  /// purge is disabled
  void doAutoPurge() {
    if (kAutoPurge) {
      doPurge();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Copies the contents of the given cache into this cache, assumes this cache is empty
  void doEmptyCopyFrom(const LruCache& cache) {
    SW_ASSERT(empty());
    if (cache.empty())
      return;

    auto iter = cache.cendOrdered();
    do {
      --iter;
      const auto& key = iter.key();
      const auto& value = iter.value();
      list_.push_front(std::make_pair(iter.key(), iter.value()));
      map_.insert(std::make_pair(iter.key(), list_.begin()));
    } while (iter != cache.cbeginOrdered());
  }

private:
  /// The map will hold list iterators. Note that list iterators don't invalidate
  /// unless you erase the underlying item. They're effectively like having a node pointer
  MapType map_;
  ListType list_;
  sizex maxSize_ = 10;

  friend UnorderedIterator;
  friend ConstUnorderedIterator;
  friend OrderedIterator;
  friend ConstOrderedIterator;
};

namespace lru_detail {

////////////////////////////////////////////////////////////////////////////////
/// Accessing an element using the unordered iterator won't change the LRU ordering
/// Certain operations take an UnorderedIterator because it's more efficient for
/// those operations than the ordered iterator.
template <typename CacheType>
class UnorderedIterator {
public:
  using Key = typename CacheType::KeyType;
  using Value = typename CacheType::ValueType;
  using MapIter = typename CacheType::MapIter;

  UnorderedIterator& operator++() {
    ++iter_;
    return *this;
  }

  UnorderedIterator operator++(int) & {  // NOLINT const return
    UnorderedIterator tmp = *this;
    operator++();
    return tmp;
  }

  UnorderedIterator& operator--() {
    --iter_;
    return *this;
  }

  UnorderedIterator operator--(int) & {  // NOLINT const return
    UnorderedIterator tmp = *this;
    operator--();
    return tmp;
  }

  const Key& key() const { return CacheType::getKey(iter_); }
  Value& value() { return CacheType::getValue(iter_); }
  const Value& value() const { return CacheType::getValue(iter_); }
  Value& operator*() { return CacheType::getValue(iter_); }
  const Value& operator*() const { return CacheType::getValue(iter_); }

  friend bool operator==(const UnorderedIterator& i1, const UnorderedIterator& i2) {
    return i1.iter_ == i2.iter_;
  }
  friend bool operator!=(const UnorderedIterator& i1, const UnorderedIterator& i2) {
    return i1.iter_ != i2.iter_;
  }

private:
  explicit UnorderedIterator(MapIter iter) noexcept : iter_(iter) {}
  MapIter iter_;
  friend CacheType;
  friend class ConstUnorderedIterator<CacheType>;
};

////////////////////////////////////////////////////////////////////////////////
/// Accessing an element using the unordered iterator won't change the LRU ordering
/// Certain operations take an UnorderedIterator because it's more efficient for
/// those operations than the ordered iterator.
template <typename CacheType>
class ConstUnorderedIterator {
public:
  using Key = typename CacheType::KeyType;
  using Value = typename CacheType::ValueType;
  using ConstMapIter = typename CacheType::ConstMapIter;

  // Allow direct conversion from non-const iterator
  ConstUnorderedIterator(UnorderedIterator<CacheType> iter) : iter_(iter.iter_) {}

  ConstUnorderedIterator& operator++() {
    ++iter_;
    return *this;
  }

  ConstUnorderedIterator operator++(int) & {  // NOLINT const return
    ConstUnorderedIterator tmp = *this;
    operator++();
    return tmp;
  }

  ConstUnorderedIterator& operator--() {
    --iter_;
    return *this;
  }

  ConstUnorderedIterator operator--(int) & {  // NOLINT const return
    ConstUnorderedIterator tmp = *this;
    operator--();
    return tmp;
  }

  const Key& key() const { return CacheType::getKey(iter_); }
  const Value& value() const { return CacheType::getValue(iter_); }
  const Value& operator*() const { return CacheType::getValue(iter_); }

  friend bool operator==(const ConstUnorderedIterator& i1, const ConstUnorderedIterator& i2) {
    return i1.iter_ == i2.iter_;
  }
  friend bool operator!=(const ConstUnorderedIterator& i1, const ConstUnorderedIterator& i2) {
    return i1.iter_ != i2.iter_;
  }

private:
  explicit ConstUnorderedIterator(ConstMapIter iter) noexcept : iter_(iter) {}
  ConstMapIter iter_;
  friend CacheType;
};

////////////////////////////////////////////////////////////////////////////////
/// Accessing an element using the ordered iterator won't change the LRU ordering
template <typename CacheType>
class OrderedIterator {
public:
  using Key = typename CacheType::KeyType;
  using Value = typename CacheType::ValueType;
  using ListIter = typename CacheType::ListIter;

  OrderedIterator& operator++() {
    ++iter_;
    return *this;
  }

  OrderedIterator operator++(int) & {  // NOLINT const return
    OrderedIterator tmp = *this;
    operator++();
    return tmp;
  }

  OrderedIterator& operator--() {
    --iter_;
    return *this;
  }

  OrderedIterator operator--(int) & {  // NOLINT const return
    OrderedIterator tmp = *this;
    operator--();
    return tmp;
  }

  const Key& key() const { return CacheType::getKey(iter_); }
  Value& value() { return CacheType::getValue(iter_); }
  const Value& value() const { return CacheType::getValue(iter_); }
  Value& operator*() { return CacheType::getValue(iter_); }
  const Value& operator*() const { return CacheType::getValue(iter_); }

  friend bool operator==(const OrderedIterator& i1, const OrderedIterator& i2) {
    return i1.iter_ == i2.iter_;
  }
  friend bool operator!=(const OrderedIterator& i1, const OrderedIterator& i2) {
    return i1.iter_ != i2.iter_;
  }

private:
  explicit OrderedIterator(ListIter iter) noexcept : iter_(iter) {}
  ListIter iter_;

  friend CacheType;
  friend class ConstOrderedIterator<CacheType>;
};

////////////////////////////////////////////////////////////////////////////////
/// Accessing an element using the ordered iterator won't change the LRU ordering
template <typename CacheType>
class ConstOrderedIterator {
public:
  using Key = typename CacheType::KeyType;
  using Value = typename CacheType::ValueType;
  using ConstListIter = typename CacheType::ConstListIter;

  // Allow direct conversion from non-const iterator
  ConstOrderedIterator(OrderedIterator<CacheType> iter) : iter_(iter.iter_) {}

  ConstOrderedIterator& operator++() {
    ++iter_;
    return *this;
  }

  ConstOrderedIterator operator++(int) & {  // NOLINT const return
    ConstOrderedIterator tmp = *this;
    operator++();
    return tmp;
  }

  ConstOrderedIterator& operator--() {
    --iter_;
    return *this;
  }

  ConstOrderedIterator operator--(int) & {  // NOLINT const return
    ConstOrderedIterator tmp = *this;
    operator--();
    return tmp;
  }

  const Key& key() const { return CacheType::getKey(iter_); }
  const Value& value() const { return CacheType::getValue(iter_); }
  const Value& operator*() const { return CacheType::getValue(iter_); }

  friend bool operator==(const ConstOrderedIterator& i1, const ConstOrderedIterator& i2) {
    return i1.iter_ == i2.iter_;
  }
  friend bool operator!=(const ConstOrderedIterator& i1, const ConstOrderedIterator& i2) {
    return i1.iter_ != i2.iter_;
  }

private:
  explicit ConstOrderedIterator(ConstListIter iter) noexcept : iter_(iter) {}
  ConstListIter iter_;

  friend CacheType;
};

}  // namespace lru_detail

}  // namespace sw
