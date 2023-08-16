#include <iostream>

template<size_t N>
class StackStorage {
 public:
  StackStorage() : start_(&container_[0]) {}

  void* alloc(size_t align, size_t size) {
    if (size_t(start_) % align != 0) {
      start_ -= size_t(start_) % align + align;
    }
    start_ += size;
    return &start_[-size];
  }
 private:
  char* start_;
  char container_[N];
};

template<typename T, size_t N>
class StackAllocator {
 public:
  using value_type = T;

  template<typename U, size_t Size>
  friend class StackAllocator;

  template<typename U>
  struct rebind { typedef StackAllocator<U, N> other; };

  StackAllocator(const StackAllocator &allocator) = default;
  ~StackAllocator() = default;

  template<typename U>
  StackAllocator(const StackAllocator<U, N> &allocator) : storage(allocator.storage) {}
  StackAllocator(StackStorage<N> &stack_storage) : storage(&stack_storage) {}

  T* allocate(const size_t size) const {
    return reinterpret_cast<T*>(storage->alloc(alignof(T), size * sizeof(T)));
  }
  void deallocate(T*, size_t) const {}

  bool operator==(const StackAllocator &allocator) const { return storage == allocator.storage; }
  bool operator!=(const StackAllocator &allocator) const { return storage != allocator.storage; }
 private:
  StackStorage<N>* storage = nullptr;
};

template<typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
  };
  struct Node : BaseNode {
    T value;
    explicit Node(const T& val) : value(val) {}
    Node() = default;
  };

  using allocator = typename Allocator::template rebind<Node>::other;
  using nodes_allocator = std::allocator_traits<allocator>;
  using values_allocator = std::allocator_traits<Allocator>;

  template<bool isConst>
  class Iterator {
    template<typename U, typename Alloc>
    friend class List;
   public:
    using value_type = std::conditional_t<isConst, const T, T>;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_traits = std::iterator_traits<Iterator<isConst>>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = int;

    Iterator(BaseNode *node) : node(node) {}
    Iterator(const Iterator<false> &iter) : node(iter.node) {}

    Iterator &operator=(const Iterator<false> &other_iter) {
      node = other_iter.node;
      return *this;
    }

    Iterator &operator++() {
      node = node->next;
      return *this;
    }

    Iterator &operator--() {
      node = node->prev;
      return *this;
    }

    Iterator operator++(int) {
      Iterator copy = *this;
      ++(*this);
      return copy;
    }

    Iterator operator--(int) {
      Iterator copy = *this;
      --(*this);
      return copy;
    }

    bool operator==(const Iterator &it) const { return node == it.node; }
    bool operator!=(const Iterator &it) const { return node != it.node; }
    reference operator*() const { return static_cast<Node*>(node)->value; }
    pointer operator->() const { return &static_cast<Node*>(node)->value; }
   private:
    BaseNode *node;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<Iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

  iterator begin() const { return iterator(fake_node->next); }
  iterator end() const { return iterator(fake_node); }
  const_iterator cbegin() const { return const_iterator(fake_node->next); }
  const_iterator cend() const { return const_iterator(fake_node); }
  reverse_iterator rbegin() const{ return reverse_iterator((fake_node)); }
  reverse_iterator rend() const { return reverse_iterator(fake_node); }
  const_reverse_iterator crbegin() const { return const_reverse_iterator((fake_node)); }
  const_reverse_iterator crend() const { return const_reverse_iterator(fake_node); }

  List() : allocator_(Allocator()), fake_node(nodes_allocator::allocate(allocator_, 1)) {
    fake_node->next = fake_node;
    fake_node->prev = fake_node;
  }

  List(allocator other_alloc) : allocator_(values_allocator::select_on_container_copy_construction(other_alloc)), fake_node(nodes_allocator::allocate(allocator_, 1)) {
    fake_node->next = fake_node;
    fake_node->prev = fake_node;
  }

  List(size_t size, Allocator other_alloc = Allocator()) : List(other_alloc) {
    for (size_t i = 0; i < size; ++i) {
      push_back();
    }
  }

  List(size_t size, const T &value, Allocator other_alloc = Allocator()) : List(other_alloc) {
    for (size_t i = 0; i < size; ++i) {
      push_back(value);
    }
  }

  List(const List &other_list) : List(values_allocator::select_on_container_copy_construction(other_list.allocator_)) {
    BaseNode* current_node = other_list.fake_node->next;
    for (size_t i = 0; i < other_list.size(); ++i) {
      push_back(static_cast<Node*>(current_node)->value);
      current_node = current_node->next;
    }
  }

  ~List() {
    while (size_ > 0) {
      pop_back();
    }
    nodes_allocator::deallocate(allocator_, fake_node, 1);
  }

  List& operator=(const List& other_list) {
    size_t old_size = size_;
    if (values_allocator::propagate_on_container_copy_assignment::value) {
      allocator_ = other_list.allocator_;
    } else {
      allocator_ = values_allocator::select_on_container_copy_construction(other_list.allocator_);
    }
    try {
      BaseNode* current_node = other_list.fake_node->next;
      for (size_t i = 0; i < other_list.size(); ++i) {
        push_back(static_cast<Node*>(current_node)->value);
        current_node = current_node->next;
      }
    } catch (...) {
      while (size_ > old_size) {
        pop_back();
      }
    }
    while (old_size > 0) {
      --old_size;
      pop_front();
    }
    return *this;
  }

  void insert(const_iterator iter, const T &value) {
    Node *new_node = static_cast<Node*>(allocate_node(value));
    Node *current_node = static_cast<Node*>(iter.node);
    new_node->next = current_node;
    new_node->prev = current_node->prev;
    current_node->prev = new_node;
    new_node->prev->next = new_node;
    ++size_;
  }

  void erase(const_iterator iter) {
    Node *deleted_node = static_cast<Node*>(iter.node);
    deleted_node->prev->next = deleted_node->next;
    deleted_node->next->prev = deleted_node->prev;
    deallocate_node(deleted_node);
    --size_;
  }

  size_t size() const { return size_; }
  allocator get_allocator() const { return allocator_; }
  void pop_back() { erase(--cend()); }
  void pop_front() { erase(cbegin()); }
  void push_front(const T& value) { insert(cbegin(), value); }
  void push_back(const T& value) { insert(cend(), value); }

 private:
  size_t size_ = 0;
  allocator allocator_;
  Node *fake_node;
  void push_back() {
    Node *new_node = allocate_node();
    new_node->prev = fake_node->prev;
    new_node->next = fake_node;
    fake_node->prev->next = new_node;
    fake_node->prev = new_node;
    ++size_;
  }

  Node* allocate_node(const T& value) {
    Node* node = nodes_allocator::allocate(allocator_, 1);
    try {
      nodes_allocator::construct(allocator_, node, value);
    } catch (...) {
      nodes_allocator::deallocate(allocator_, node, 1);
      throw;
    }
    return node;
  }

  Node* allocate_node() {
    Node* node = nodes_allocator::allocate(allocator_, 1);
    try {
      nodes_allocator::construct(allocator_, node);
    } catch (...) {
      nodes_allocator::deallocate(allocator_, node, 1);
      throw;
    }
    return node;
  }

  void deallocate_node(Node* node) {
    nodes_allocator::destroy(allocator_, node);
    nodes_allocator::deallocate(allocator_, node, 1);
  }
};

