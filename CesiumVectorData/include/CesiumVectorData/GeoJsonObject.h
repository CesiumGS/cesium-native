#pragma once

#include "GeoJsonObjectTypes.h"

#include <stack>
#include <vector>

namespace CesiumVectorData {

struct GeoJsonObjectIterator;
struct ConstGeoJsonObjectIterator;
template <typename SingleT, typename MultiT, typename ValueT>
struct ConstGeoJsonPrimitiveIterator;
template <typename ObjectType> struct ConstGeoJsonObjectTypeIterator;

using ConstGeoJsonPointIterator =
    ConstGeoJsonPrimitiveIterator<GeoJsonPoint, GeoJsonMultiPoint, glm::dvec3>;
using ConstGeoJsonLineStringIterator = ConstGeoJsonPrimitiveIterator<
    GeoJsonLineString,
    GeoJsonMultiLineString,
    std::vector<glm::dvec3>>;
using ConstGeoJsonPolygonIterator = ConstGeoJsonPrimitiveIterator<
    GeoJsonPolygon,
    GeoJsonMultiPolygon,
    std::vector<std::vector<glm::dvec3>>>;

/**
 * @brief A wrapper around an object in a GeoJSON document.
 */
struct GeoJsonObject {
  template <typename IteratorType> struct IteratorProvider {
    IteratorProvider(const GeoJsonObject* pObject) : _pObject(pObject) {}

    IteratorType begin() { return IteratorType(*_pObject); }

    IteratorType end() { return IteratorType(); }

  private:
    const GeoJsonObject* _pObject;
  };

  GeoJsonObjectIterator begin();
  GeoJsonObjectIterator end();
  ConstGeoJsonObjectIterator begin() const;
  ConstGeoJsonObjectIterator end() const;

  IteratorProvider<ConstGeoJsonPointIterator> points() const {
    return IteratorProvider<ConstGeoJsonPointIterator>(this);
  }

  IteratorProvider<ConstGeoJsonLineStringIterator> lines() const {
    return IteratorProvider<ConstGeoJsonLineStringIterator>(this);
  }

  IteratorProvider<ConstGeoJsonPolygonIterator> polygons() const {
    return IteratorProvider<ConstGeoJsonPolygonIterator>(this);
  }

  template <typename ObjectType>
  IteratorProvider<ConstGeoJsonObjectTypeIterator<ObjectType>>
  allOfType() const {
    return IteratorProvider<ConstGeoJsonObjectTypeIterator<ObjectType>>(this);
  }

  /**
   * @brief Returns the \ref GeoJsonObjectType that this \ref GeoJsonObject
   * is wrapping.
   */
  GeoJsonObjectType getType() const;

  /**
   * @brief Returns whether the `value` of this \ref GeoJsonObject is of the
   * given type.
   */
  template <typename T> bool containsType() const {
    return std::holds_alternative<T>(this->value);
  }

  /**
   * @brief Obtains a reference to the value of this \ref GeoJsonObject if the
   * value is of the given type.
   *
   * @exception std::bad_variant_access if the value is not of type `T`
   */
  template <typename T> const T& get() const {
    return std::get<T>(this->value);
  }

  /**
   * @brief Obtains a reference to the value of this \ref GeoJsonObject if the
   * value is of the given type.
   *
   * @exception std::bad_variant_access if the value is not of type `T`
   */
  template <typename T> T& get() { return std::get<T>(this->value); }

  /**
   * @brief Obtains a pointer to the value of this \ref GeoJsonObject if the
   * value is of the given type. Otherwise `nullptr` is returned.
   */
  template <typename T> const T* getIf() const {
    return std::get_if<T>(&this->value);
  }

  /**
   * @brief Obtains a pointer to the value of this \ref GeoJsonObject if the
   * value is of the given type. Otherwise `nullptr` is returned.
   */
  template <typename T> T* getIf() { return std::get_if<T>(&this->value); }

  /**
   * @brief Applies the visitor `visitor` to the value variant.
   */
  template <typename Visitor, typename RetVal> RetVal visit(Visitor&& visitor) {
    return std::visit(visitor, this->value);
  }

  /**
   * @brief A variant containing the GeoJSON object.
   */
  GeoJsonObjectVariant value;
};

/**
 * @brief Iterates over a \ref GeoJsonObject and all of its children.
 */
struct GeoJsonObjectIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = GeoJsonObject;
  using pointer = GeoJsonObject*;
  using reference = GeoJsonObject&;

  reference operator*() const { return *_pCurrentObject; }
  pointer operator->() { return _pCurrentObject; }

  /**
   * @brief Attempts to find the Feature that contains the current item the
   * iterator is pointing to.
   *
   * If the iterator is pointing to a Feature, that Feature will be returned.
   */
  GeoJsonObject* getFeature() const {
    for (auto it = this->_objectStack.rbegin(); it != this->_objectStack.rend();
         ++it) {
      if ((*it)->containsType<GeoJsonFeature>()) {
        return *it;
      }
    }

    return nullptr;
  }

  bool isEnded() const {
    return this->_nextPosStack.empty() && this->_objectStack.empty() &&
           this->_pCurrentObject == nullptr;
  }

  GeoJsonObjectIterator& operator++() {
    this->iterate();
    return *this;
  }
  GeoJsonObjectIterator operator++(int) {
    GeoJsonObjectIterator tmp = *this;
    this->iterate();
    return tmp;
  }
  friend bool
  operator==(const GeoJsonObjectIterator& a, const GeoJsonObjectIterator& b) {
    if (a._pCurrentObject != b._pCurrentObject) {
      return false;
    }

    if (!a._objectStack.empty() && !b._objectStack.empty()) {
      if (a._objectStack.back() != b._objectStack.back()) {
        return false;
      }
    }

    if (!a._nextPosStack.empty() && !b._nextPosStack.empty()) {
      if (a._nextPosStack.top() != b._nextPosStack.top()) {
        return false;
      }
    }

    return a._objectStack.empty() == b._objectStack.empty() &&
           a._nextPosStack.empty() == b._nextPosStack.empty();
  }
  friend bool
  operator!=(const GeoJsonObjectIterator& a, const GeoJsonObjectIterator& b) {
    return !(a == b);
  }

  GeoJsonObjectIterator(GeoJsonObject& rootObject)
      : _objectStack(), _nextPosStack(), _pCurrentObject(nullptr) {
    this->_objectStack.emplace_back(&rootObject);
    this->_nextPosStack.emplace(-1);
    this->iterate();
  }

  GeoJsonObjectIterator()
      : _objectStack(), _nextPosStack(), _pCurrentObject(nullptr) {}

private:
  void handleChild(GeoJsonObject& child) {
    this->_pCurrentObject = &child;

    if (child.containsType<GeoJsonGeometryCollection>() ||
        child.containsType<GeoJsonFeatureCollection>() ||
        child.containsType<GeoJsonFeature>()) {
      this->_nextPosStack.emplace(-1);
      this->_objectStack.emplace_back(&child);
    }
  }

  void iterate() {
    _pCurrentObject = nullptr;
    while (!this->_objectStack.empty() && !this->_nextPosStack.empty() &&
           this->_pCurrentObject == nullptr) {
      GeoJsonObject* pNext = _objectStack.back();
      if (this->_nextPosStack.top() == -1) {
        this->_pCurrentObject = pNext;
        this->_nextPosStack.top()++;
        continue;
      } else if (std::holds_alternative<GeoJsonGeometryCollection>(
                     pNext->value)) {
        GeoJsonGeometryCollection& collection =
            std::get<GeoJsonGeometryCollection>(pNext->value);
        if ((size_t)this->_nextPosStack.top() >= collection.geometries.size()) {
          // No children left
          this->_objectStack.pop_back();
          this->_nextPosStack.pop();
          continue;
        }

        GeoJsonObject& child =
            collection.geometries[(size_t)this->_nextPosStack.top()];
        this->_nextPosStack.top()++;
        this->handleChild(child);
        continue;
      } else if (std::holds_alternative<GeoJsonFeatureCollection>(
                     pNext->value)) {
        GeoJsonFeatureCollection& collection =
            std::get<GeoJsonFeatureCollection>(pNext->value);
        if ((size_t)this->_nextPosStack.top() >= collection.features.size()) {
          // No children left
          this->_objectStack.pop_back();
          this->_nextPosStack.pop();
          continue;
        }

        GeoJsonObject& child =
            collection.features[(size_t)this->_nextPosStack.top()];
        this->_nextPosStack.top()++;
        this->handleChild(child);
        continue;
      } else if (std::holds_alternative<GeoJsonFeature>(pNext->value)) {
        GeoJsonFeature& feature = std::get<GeoJsonFeature>(pNext->value);
        if ((size_t)this->_nextPosStack.top() >= 1) {
          // Feature only has one child
          this->_objectStack.pop_back();
          this->_nextPosStack.pop();
          continue;
        }

        this->_nextPosStack.top()++;
        this->handleChild(*feature.geometry);
        continue;
      } else {
        // This object was a dud, try another
        this->_nextPosStack.pop();
        this->_objectStack.pop_back();
      }
    }
  }

  std::vector<GeoJsonObject*> _objectStack;
  std::stack<int64_t> _nextPosStack;
  GeoJsonObject* _pCurrentObject;

  friend struct ConstGeoJsonObjectIterator;
};

struct ConstGeoJsonObjectIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = GeoJsonObject;
  using pointer = const GeoJsonObject*;
  using reference = const GeoJsonObject&;

  reference operator*() const { return *this->_it; }
  pointer operator->() { return this->_it._pCurrentObject; }

  const GeoJsonObject* getFeature() { return this->_it.getFeature(); }

  ConstGeoJsonObjectIterator& operator++() {
    ++this->_it;
    return *this;
  }
  ConstGeoJsonObjectIterator operator++(int) {
    ConstGeoJsonObjectIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  friend bool operator==(
      const ConstGeoJsonObjectIterator& a,
      const ConstGeoJsonObjectIterator& b) {
    return a._it == b._it;
  }
  friend bool operator!=(
      const ConstGeoJsonObjectIterator& a,
      const ConstGeoJsonObjectIterator& b) {
    return a._it != b._it;
  }

  ConstGeoJsonObjectIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {}
  ConstGeoJsonObjectIterator() = default;

private:
  GeoJsonObjectIterator _it;
};

template <typename SingleT, typename MultiT, typename ValueT>
struct ConstGeoJsonPrimitiveIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ValueT;
  using pointer = const ValueT*;
  using reference = const ValueT&;

  reference operator*() const {
    const GeoJsonMultiPoint* pMultiPoint =
        (*this->_it).getIf<GeoJsonMultiPoint>();
    if (pMultiPoint) {
      return pMultiPoint->coordinates[this->_currentMultiIdx];
    }

    return (*this->_it).get<GeoJsonPoint>().coordinates;
  }
  pointer operator->() { return &**this; }

  ConstGeoJsonPrimitiveIterator& operator++() {
    this->iterate();
    return *this;
  }
  ConstGeoJsonPrimitiveIterator operator++(int) {
    ConstGeoJsonPrimitiveIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  friend bool operator==(
      const ConstGeoJsonPrimitiveIterator& a,
      const ConstGeoJsonPrimitiveIterator& b) {
    return a._it == b._it;
  }
  friend bool operator!=(
      const ConstGeoJsonPrimitiveIterator& a,
      const ConstGeoJsonPrimitiveIterator& b) {
    return a._it != b._it;
  }

  ConstGeoJsonPrimitiveIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {
    if (!_it.isEnded() &&
        !(_it->containsType<SingleT>() || _it->containsType<MultiT>())) {
      this->iterate();
    }
  }
  ConstGeoJsonPrimitiveIterator() = default;

private:
  void iterate() {
    if (!this->_it.isEnded() && this->_it->containsType<MultiT>()) {
      const MultiT& multi = this->_it->get<MultiT>();
      if ((int64_t)this->_currentMultiIdx <
          (int64_t)(multi.coordinates.size() - 1)) {
        this->_currentMultiIdx++;
        return;
      }
    }

    this->_currentMultiIdx = 0;
    do {
      ++this->_it;
    } while (!this->_it.isEnded() &&
             !(this->_it->containsType<SingleT>() ||
               (this->_it->containsType<MultiT>() &&
                !this->_it->get<MultiT>().coordinates.empty())));
  }

  GeoJsonObjectIterator _it;
  size_t _currentMultiIdx = 0;
};

template <typename ObjectType> struct ConstGeoJsonObjectTypeIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ObjectType;
  using pointer = const ObjectType*;
  using reference = const ObjectType&;

  reference operator*() const {
    return (*this->_it).get<ObjectType>().coordinates;
  }
  pointer operator->() { return &**this; }

  ConstGeoJsonObjectTypeIterator& operator++() {
    do {
      ++this->_it;
    } while (!this->_it.isEnded() && this->_it->containsType<ObjectType>());
    return *this;
  }
  ConstGeoJsonObjectTypeIterator operator++(int) {
    ConstGeoJsonObjectTypeIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  friend bool operator==(
      const ConstGeoJsonObjectTypeIterator& a,
      const ConstGeoJsonObjectTypeIterator& b) {
    return a._it == b._it;
  }
  friend bool operator!=(
      const ConstGeoJsonObjectTypeIterator& a,
      const ConstGeoJsonObjectTypeIterator& b) {
    return a._it != b._it;
  }

  ConstGeoJsonObjectTypeIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {}
  ConstGeoJsonObjectTypeIterator() = default;

private:
  GeoJsonObjectIterator _it;
  size_t _currentMultiPointIdx = 0;
};
} // namespace CesiumVectorData