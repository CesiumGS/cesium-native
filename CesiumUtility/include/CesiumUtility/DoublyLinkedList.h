#pragma once

#include <cstddef>

namespace CesiumUtility {

/**
 * @brief Contains the previous and next pointers for an element in
 * a {@link DoublyLinkedList}.
 */
template <class T> class DoublyLinkedListPointers final {
public:
  /**
   * @brief Default constructor.
   */
  DoublyLinkedListPointers() noexcept : pNext(nullptr), pPrevious(nullptr) {}

  /**
   * @brief Copy constructor.
   *
   * @param rhs The other instance.
   */
  // Following the example of boost::instrusive::list's list_member_hook, the
  // copy constructor and assignment operator do nothing.
  // https://www.boost.org/doc/libs/1_73_0/doc/html/boost/intrusive/list_member_hook.html
  DoublyLinkedListPointers(
      [[maybe_unused]] DoublyLinkedListPointers& rhs) noexcept
      : DoublyLinkedListPointers() {}

  /**
   * @brief Assignment operator.
   */
  DoublyLinkedListPointers&
  operator=(const DoublyLinkedListPointers& /*rhs*/) noexcept { // NOLINT
    return *this;
  }

private:
  template <
      typename TElement,
      typename TElementBase,
      DoublyLinkedListPointers<TElement>(TElementBase::*Pointers)>
  friend class DoublyLinkedListAdvanced;

  T* pNext;
  T* pPrevious;
};

/**
 * @brief A doubly-linked list.
 *
 * In this implementation, the previous and next pointers are embedded directly
 * in the data object.
 *
 * @tparam T The data object type.
 * @tparam (T::*Pointers) A member pointer to the field that holds the links to
 * the previous and next nodes.
 */
template <
    typename T,
    typename TPointerBase,
    DoublyLinkedListPointers<T>(TPointerBase::*Pointers)>
class DoublyLinkedListAdvanced final {
public:
  /**
   * @brief Removes the given node from this list.
   */
  void remove(T& node) noexcept {
    DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

    if (nodePointers.pPrevious) {
      DoublyLinkedListPointers<T>& previousPointers =
          nodePointers.pPrevious->*Pointers;
      previousPointers.pNext = nodePointers.pNext;
      --this->_size;
    } else if (this->_pHead == &node) {
      this->_pHead = nodePointers.pNext;
      --this->_size;
    }

    if (nodePointers.pNext) {
      DoublyLinkedListPointers<T>& nextPointers = nodePointers.pNext->*Pointers;
      nextPointers.pPrevious = nodePointers.pPrevious;
    } else if (this->_pTail == &node) {
      this->_pTail = nodePointers.pPrevious;
    }

    nodePointers.pPrevious = nullptr;
    nodePointers.pNext = nullptr;
  }

  /**
   * @brief Insert the given node after the other node.
   */
  void insertAfter(T& after, T& node) noexcept {
    this->remove(node);

    DoublyLinkedListPointers<T>& afterPointers = after.*Pointers;
    DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

    nodePointers.pPrevious = &after;
    nodePointers.pNext = afterPointers.pNext;
    afterPointers.pNext = &node;

    if (nodePointers.pNext) {
      DoublyLinkedListPointers<T>& nextPointers = nodePointers.pNext->*Pointers;
      nextPointers.pPrevious = &node;
    }

    if (this->_pTail == &after) {
      this->_pTail = &node;
    }

    ++this->_size;
  }

  /**
   * @brief Insert the given node before the other node.
   */
  void insertBefore(T& before, T& node) noexcept {
    this->remove(node);

    DoublyLinkedListPointers<T>& beforePointers = before.*Pointers;
    DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

    nodePointers.pPrevious = beforePointers.pPrevious;
    nodePointers.pNext = &before;
    beforePointers.pPrevious = &node;

    if (nodePointers.pPrevious) {
      DoublyLinkedListPointers<T>& previousPointers =
          nodePointers.pPrevious->*Pointers;
      previousPointers.pNext = &node;
    }

    if (this->_pHead == &before) {
      this->_pHead = &node;
    }

    ++this->_size;
  }

  /**
   * @brief Insert the given node as the new head of the list.
   */
  void insertAtHead(T& node) noexcept {
    this->remove(node);

    if (this->_pHead) {
      (this->_pHead->*Pointers).pPrevious = &node;
      (node.*Pointers).pNext = this->_pHead;
    } else {
      this->_pTail = &node;
    }
    this->_pHead = &node;

    ++this->_size;
  }

  /**
   * @brief Insert the given node as the new tail of the list.
   */
  void insertAtTail(T& node) noexcept {
    this->remove(node);

    if (this->_pTail) {
      (this->_pTail->*Pointers).pNext = &node;
      (node.*Pointers).pPrevious = this->_pTail;
    } else {
      this->_pHead = &node;
    }
    this->_pTail = &node;

    ++this->_size;
  }

  /**
   * @brief Returns the size of this list.
   */
  size_t size() const noexcept { return this->_size; }

  /**
   * @brief Returns the head node of this list, or `nullptr` if the list is
   * empty.
   */
  T* head() noexcept { return this->_pHead; }

  /** @copydoc DoublyLinkedList::head() */
  const T* head() const noexcept { return this->_pHead; }

  /**
   * @brief Returns the tail node of this list, or `nullptr` if the list is
   * empty.
   */
  T* tail() noexcept { return this->_pTail; }

  /** @copydoc DoublyLinkedList::tail() */
  const T* tail() const noexcept { return this->_pTail; }

  /**
   * @brief Returns the next node after the given one, or `nullptr` if the given
   * node is the tail.
   */
  T* next(T& node) noexcept { return (node.*Pointers).pNext; }

  /** @copydoc DoublyLinkedList::next(T&) */
  const T* next(const T& node) const noexcept { return (node.*Pointers).pNext; }

  /**
   * @brief Returns the next node after the given one, or the head if the given
   * node is `nullptr`.
   */
  T* next(T* pNode) noexcept {
    return pNode ? this->next(*pNode) : this->_pHead;
  }

  /** @copydoc DoublyLinkedList::next(T*) */
  const T* next(const T* pNode) const noexcept {
    return pNode ? this->next(*pNode) : this->_pHead;
  }

  /**
   * @brief Returns the previous node before the given one, or `nullptr` if the
   * given node is the head.
   */
  T* previous(T& node) noexcept { return (node.*Pointers).pPrevious; }

  /** @copydoc DoublyLinkedList::previous(T&) */
  const T* previous(const T& node) const noexcept {
    return (node.*Pointers).pPrevious;
  }

  /**
   * @brief Returns the previous node before the given one, or the tail if the
   * given node is `nullptr`.
   */
  T* previous(T* pNode) {
    return pNode ? this->previous(*pNode) : this->_pTail;
  }

  /** @copydoc DoublyLinkedList::previous(T*) */
  const T* previous(const T* pNode) const noexcept {
    return pNode ? this->previous(*pNode) : this->_pTail;
  }

  /**
   * @brief Determines if this list contains a given node in constant time. In
   * order to avoid a full list scan, this method assumes that if the node has
   * any next or previous node, then it is contained in this list. Do not use
   * this method to determine which of multiple lists contain this node.
   *
   * @param node The node to check.
   * @return True if this node is the head of the list, or if the node has next
   * or previous nodes. False if the node does not have next or previous nodes
   * and it is not the head of this list.
   */
  bool contains(const T& node) const {
    return this->next(node) != nullptr || this->previous(node) != nullptr ||
           this->_pHead == &node;
  }

private:
  size_t _size = 0;
  T* _pHead = nullptr;
  T* _pTail = nullptr;
};

/**
 * @brief An intrusive doubly-linked list.
 */
template <typename T, DoublyLinkedListPointers<T>(T::*Pointers)>
using DoublyLinkedList = DoublyLinkedListAdvanced<T, T, Pointers>;

} // namespace CesiumUtility
