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
/// The structure of the cache will be a double-linked list used in conjunction with hashmap.
/// You should be able to lookup anything from the cache in O(1). Items that are older can get
/// purged, thus we also need to maintain an ordering. The double-linked list maintains the order
/// while the hashmap points to nodes in the list allowing O(1) lookup
///
namespace lru_detail {

////////////////////////////////////////////////////////////////////////////////
/// Custom allocator is TODO if needed
/// Pretty much everything not in here is TODO if needed.
template <typename T>
class List {
public:
  // This list is intended to expose Nodes. Doing something like C++17 node-handles sounds
  // like a good idea if this was a public class. Investigate when needed
  struct Node;
  using NodePtr = std::unique_ptr<Node>;

  struct Iterator;
  struct ConstIterator;

  /// std-lib style types
  using iterator = Iterator;
  using const_iterator = ConstIterator;
  using value_type = T;
  using size_type = sizex;
  using difference_type = ptrdiffx;
  using reference = value_type&;
  using const_reference = const value_type&;
  using allocator = std::allocator<T>;  // custom is TODO

  // Interesting problem here. Looking at stdlib list class, it's using 'rebind' which
  // is related to std::allocator. But, rebind is going away. Looks like polymorphic
  // allocator will be the way in the future.
  using node_allocator = std::allocator<Node>;  // custom is TODO

public:
  List() = default;

  List(const List&) = delete;
  List& operator=(const List&) = delete;
  List(List&&) = delete;
  List& operator=(List&&) = delete;

  ////////////////////////////////////////////////////////////////////////////////
  ~List() {
    // TODO: need to manually delete the list
    Node* end = endNode();
    Node* node = end->next;
    while (node != end) {
      Node* deleteMe = node;
      node = node->next;
      deleteNode(deleteMe);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// About the node... was considering using std::unique_ptr for 'next' thus gaining
  /// auto-magic deletion. But, the concern there is stack-depth. A large list will
  /// end up creating a massive stack during deletion. So just going to use good
  /// ol' fashion manual allocation/deletion.
  struct Node {
    ~Node() = default;
    Node() = default;
    Node(Node* prev, Node* next, T&& v) : prev(prev), next(next), value(std::move(v)) {}
    Node(Node* prev, Node* next, const T& v = T()) : prev(prev), next(next), value(std::move(v)) {}

    Node(const Node& node) = delete;
    Node& operator=(const Node& node) = delete;

    // TODO maybe?
    Node(Node&& node) = default;
    Node& operator=(Node&& node) = default;

    Node* prev = nullptr;
    Node* next = nullptr;
    T value;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// This iterator isn't really useful for the LruCache class, but it is helpful for
  /// testing, so I'll keep it around.
  class Iterator {
  public:
    Iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      operator++();
      return tmp;
    }

    Iterator& operator--() {
      // If the iterator is at the end, then it goes back to the tail
      node_ = node_->prev;
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      operator--();
      return tmp;
    }

    T& operator*() { return node_->value; }
    const T& operator*() const { return node_->value; }

    friend bool operator==(const Iterator& i1, const Iterator& i2) { return i1.node_ == i2.node_; }
    friend bool operator!=(const Iterator& i1, const Iterator& i2) { return i1.node_ != i2.node_; }

  private:
    explicit Iterator(Node* node) noexcept : node_(node) {}
    Node* node_ = nullptr;
    friend class List;
  };

  ////////////////////////////////////////////////////////////////////////////////
  bool empty() const { return headNode() != nullptr; }

  ////////////////////////////////////////////////////////////////////////////////
  Iterator begin() noexcept { return Iterator(headNode()); }
  Iterator end() noexcept { return Iterator(endNode()); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Remove the given node from the list. All memory associated with the node will
  /// be deleted
  void erase(Node*&& node) {
    SW_ASSERT(node != nullptr);

    // First stitch out the node from where it's at
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // And delete it
    deleteNode(node);
  }

  ////////////////////////////////////////////////////////////////////////////////
  Node* pushBack(const T& value) {
    auto* oldTail = tailNode();
    auto* end = endNode();
    auto* newTail = makeNode(oldTail, end, value);
    oldTail->next = newTail;
    end->prev = newTail;
    return newTail;
  }

  ////////////////////////////////////////////////////////////////////////////////
  Node* pushBack(T&& value) {
    auto* oldTail = tailNode();
    auto* end = endNode();
    auto* newTail = makeNode(oldTail, end, std::move(value));
    oldTail->next = newTail;
    end->prev = newTail;
    return newTail;
  }

  ////////////////////////////////////////////////////////////////////////////////
  Node* pushFront(const T& value) {
    auto* oldHead = headNode();
    auto* end = endNode();
    auto* newHead = makeNode(end, oldHead, value);
    oldHead->prev = newHead;
    end->next = newHead;
    return newHead;
  }

  ////////////////////////////////////////////////////////////////////////////////
  Node* pushFront(T&& value) {
    auto* oldHead = headNode();
    auto* end = endNode();
    auto* newHead = makeNode(end, oldHead, std::move(value));
    oldHead->prev = newHead;
    end->next = newHead;
    return newHead;
  }

  ////////////////////////////////////////////////////////////////////////////////
  Node* frontNode() { return headNode(); }
  const Node* frontNode() const { return headNode(); }

  ////////////////////////////////////////////////////////////////////////////////
  Node* backNode() { return tailNode(); }
  const Node* backNode() const { return tailNode(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Moves the given node to the back of the list
  void moveToBack(Node* node) {
    SW_ASSERT(node != nullptr);

    // First stitch out the node from where it's at
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // Now push it to the back, updating everything
    auto* oldTail = tailNode();
    auto* end = endNode();
    oldTail->next = node;
    node->prev = oldTail;
    end->prev = node;
    node->next = end;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Moves the given node to the front of the list
  void moveToFront(Node* node) {
    SW_ASSERT(node != nullptr);

    // First stitch out the node from where it's at
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // Now push it onto the front, updating everything
    auto* oldHead = headNode();
    auto* end = endNode();
    oldHead->prev = node;
    node->next = oldHead;
    end->next = node;
    node->prev = end;
  }

private:
  Node* endNode() noexcept { return &end_; }
  Node* tailNode() noexcept { return end_.prev; }
  Node* headNode() noexcept { return end_.next; }

  /// Using a function to create nodes so we can drop an allocator in later
  template <typename... Args>
  Node* makeNode(Args&&... args) {
    Node* nodeMem = allocNode().allocate(1);
    Node* node = new (nodeMem) Node(std::forward<Args>(args)...);
    return node;
  };

  /// Delete a node that was created via 'makeNode'
  void deleteNode(Node* node) {
    if (node) {
      node->~Node();
      allocNode().deallocate(node, 1);
    }
  }

  // TODO: if we add custom allocator, this would return a reference, and we'd want to
  // use the compressed_pair thus holding the allocator as a member
  allocator alloc() const noexcept { return allocator(); }
  node_allocator allocNode() const noexcept { return node_allocator(); }

  // We only hold the "end", which is an artificial node that is conceptually
  // *after* the tail. It's also *before* the head, but that's more of an implementation
  // detail than a conceptual help. This solves the iterator "end" needs and also
  // allows that no node pointers are ever null. I originally thought it might make the
  // implementation more complex, but getting rid of null nodes has added some
  // elegance. For 'end', the nodes are defined as:
  //   end_.next points to the head of the list
  //   end_.prev points to the tail of the list
  // ALSO: Since no node pointers are null, when it's an empty list, end_.next points
  // to end_, and end_.prev point to end_.
  Node end_ = Node{endNode(), endNode()};
};

}  // namespace lru_detail

////////////////////////////////////////////////////////////////////////////////
template <typename Key, typename T>
class LruCache {
public:
  ////////////////////////////////////////////////////////////////////////////////
  /// Return the number of items in the map
  sizex size() const { return map_.size(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Is the map empty?
  bool empty() const { return map_.empty(); }

  ////////////////////////////////////////////////////////////////////////////////
  // Extract the cached value for the given key. If the value isn't in the cache, a
  // default constructed value will be created.
  T& operator[](const Key& key) {
    auto iter = map_.find(key);
    auto* node = [&]() {
      if (iter == map_.end()) {
        // No item. We needs to create a default value in that case
        // First insert a default value to the front of the list
        auto* node = list_.pushFront(T());

        // Now we can insert the node into the map. It's already in the correct order in the list
        auto result = map_.insert(std::make_pair(key, node));
        SW_ASSERT(result.second == true);
        iter = result.first;
        return node;
      }

      // The item already exists - bring it to the front of the list since it's been "used"
      auto* node = iter->second;
      list_.moveToFront(node);
      return node;
    }();

    return node->value;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Check if the given key is mapped to a value in the cache
  bool exists(const Key& key) {
    auto iter = map_.find(key);
    return iter != map_.end();
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Removed any mapped value
  sizex erase(const Key& key) {
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      return 0;
    }

    // Remove the node from the list and the map
    auto* node = iter->second;
    list_.erase(std::move(node));
    map_.erase(iter);
    return 1;
  }

#if TODO  // as needed?
  void insert(const Key& key, const T& value);
  void insert(const Key& key, T&& value);
  T& find(const Key& key);
#endif

private:
  using List = lru_detail::List<T>;
  using Node = typename List::Node;

  SysHashMap<Key, Node*> map_;
  List list_;
};

}  // namespace sw
