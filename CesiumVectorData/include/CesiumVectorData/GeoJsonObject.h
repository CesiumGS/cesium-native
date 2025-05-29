#pragma once

#include "GeoJsonObjectTypes.h"

#include <CesiumGeometry/AxisAlignedBox.h>

#include <stack>
#include <vector>

namespace CesiumVectorData {

struct GeoJsonObjectIterator;
struct ConstGeoJsonObjectIterator;
template <typename SingleT, typename MultiT, typename ValueT>
struct ConstGeoJsonPrimitiveIterator;
template <typename ObjectType> struct ConstGeoJsonObjectTypeIterator;

/**
 * @brief An iterator over all `Point` and `MultiPoint` objects in and including
 * a root GeoJSON object.
 */
using ConstGeoJsonPointIterator =
    ConstGeoJsonPrimitiveIterator<GeoJsonPoint, GeoJsonMultiPoint, glm::dvec3>;
/**
 * @brief An iterator over all `LineString` and `MultiLineString` objects in and
 * including a root GeoJSON object.
 */
using ConstGeoJsonLineStringIterator = ConstGeoJsonPrimitiveIterator<
    GeoJsonLineString,
    GeoJsonMultiLineString,
    std::vector<glm::dvec3>>;
/**
 * @brief An iterator over all `Polygon` and `MultiPolygon` objects in and
 * including a root GeoJSON object.
 */
using ConstGeoJsonPolygonIterator = ConstGeoJsonPrimitiveIterator<
    GeoJsonPolygon,
    GeoJsonMultiPolygon,
    std::vector<std::vector<glm::dvec3>>>;

/**
 * @brief A wrapper around an object in a GeoJSON document.
 */
struct GeoJsonObject {
  /**
   * @brief An object providing `begin` and `end` methods for creating iterators
   * of the given type for a \ref GeoJsonObject.
   */
  template <typename IteratorType> struct IteratorProvider {
    /**
     * @brief Returns an iterator pointing to the first element.
     */
    IteratorType begin() { return IteratorType(*_pObject); }

    /**
     * @brief Returns an iterator pointing "past the end" of all the elements.
     */
    IteratorType end() { return IteratorType(); }

  private:
    IteratorProvider(const GeoJsonObject* pObject) : _pObject(pObject) {}

    const GeoJsonObject* _pObject;
    friend struct GeoJsonObject;
  };

  /**
   * @brief Returns an iterator pointing to this object. Iterating this will
   * provide all children of this object.
   */
  GeoJsonObjectIterator begin();
  /**
   * @brief Returns an iterator pointing "past the end" of the list of children
   * of this object.
   */
  GeoJsonObjectIterator end();

  /**
   * @brief Returns an iterator pointing to this object. Iterating this will
   * provide all children of this object.
   */
  ConstGeoJsonObjectIterator begin() const;
  /**
   * @brief Returns an iterator pointing "past the end" of the list of children
   * of this object.
   */
  ConstGeoJsonObjectIterator end() const;

  /**
   * @brief Allows iterating over all points defined in this object or any child
   * objects. This will include both `Point` objects and `MultiPoint` objects.
   */
  IteratorProvider<ConstGeoJsonPointIterator> points() const;

  /**
   * @brief Allows iterating over all lines defined in this object or any child
   * objects. This will include both `LineString` objects and `MultiLineString`
   * objects.
   */
  IteratorProvider<ConstGeoJsonLineStringIterator> lines() const;

  /**
   * @brief Allows iterating over all polygons defined in this object or any
   * child objects. This will include both `Polygon` objects and `MultiPolygon`
   * objects.
   */
  IteratorProvider<ConstGeoJsonPolygonIterator> polygons() const;

  /**
   * @brief Returns all \ref GeoJsonObject values matching the given type in
   * this object or in any children.
   */
  template <typename ObjectType>
  IteratorProvider<ConstGeoJsonObjectTypeIterator<ObjectType>>
  allOfType() const {
    return IteratorProvider<ConstGeoJsonObjectTypeIterator<ObjectType>>(this);
  }

  /**
   * @brief Returns the bounding box defined in the GeoJSON for this object, if
   * any.
   */
  const std::optional<CesiumGeometry::AxisAlignedBox>& getBoundingBox() const;

  /**
   * @brief Returns the bounding box defined in the GeoJSON for this object, if
   * any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox>& getBoundingBox();

  /**
   * @brief Returns the \ref CesiumUtility::JsonValue::Object containing any
   * foreign members on this GeoJSON object.
   */
  const CesiumUtility::JsonValue::Object& getForeignMembers() const;

  /**
   * @brief Returns the \ref CesiumUtility::JsonValue::Object containing any
   * foreign members on this GeoJSON object.
   */
  CesiumUtility::JsonValue::Object& getForeignMembers();

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
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = GeoJsonObject;
  using pointer = GeoJsonObject*;
  using reference = GeoJsonObject&;
  //! @endcond

  /** @brief Returns a reference to the current object. */
  reference operator*() const { return *_pCurrentObject; }
  /** @brief Returns a pointer to the current object. */
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

  /**
   * @brief Returns `true` if this is an "end" iterator (points past the end of
   * all objects).
   */
  bool isEnded() const {
    return this->_nextPosStack.empty() && this->_objectStack.empty() &&
           this->_pCurrentObject == nullptr;
  }

  /**
   * @brief Iterates to the next \ref GeoJsonObject, returning this modified
   * iterator.
   */
  GeoJsonObjectIterator& operator++() {
    this->iterate();
    return *this;
  }
  /**
   * @brief Iterates to the next \ref GeoJsonObject, returning the previous
   * state of the iterator.
   */
  GeoJsonObjectIterator operator++(int) {
    GeoJsonObjectIterator tmp = *this;
    this->iterate();
    return tmp;
  }

  /**
   * @brief Checks if two \ref GeoJsonObjectIterator iterators are equal.
   */
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

  /**
   * @brief Checks if two \ref GeoJsonObjectIterator iterators are not equal.
   */
  friend bool
  operator!=(const GeoJsonObjectIterator& a, const GeoJsonObjectIterator& b) {
    return !(a == b);
  }

  /**
   * @brief Creates a new \ref GeoJsonObjectIterator with the given \ref
   * GeoJsonObject as the root object.
   *
   * @param rootObject The root object of the new iterator. This will be the
   * first object returned.
   */
  GeoJsonObjectIterator(GeoJsonObject& rootObject)
      : _objectStack(), _nextPosStack(), _pCurrentObject(nullptr) {
    this->_objectStack.emplace_back(&rootObject);
    this->_nextPosStack.emplace(-1);
    this->iterate();
  }

  /**
   * @brief Creates a new \ref GeoJsonObjectIterator without any \ref
   * GeoJsonObject. This is equivalent to an "end" iterator.
   */
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

/**
 * @brief The `const` equivalent of \ref GeoJsonObjectIterator.
 */
struct ConstGeoJsonObjectIterator {
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = GeoJsonObject;
  using pointer = const GeoJsonObject*;
  using reference = const GeoJsonObject&;
  //! @endcond

  /** @brief Returns a reference to the current object. */
  reference operator*() const { return *this->_it; }
  /** @brief Returns a pointer to the current object. */
  pointer operator->() { return this->_it._pCurrentObject; }

  /**
   * @brief Attempts to find the Feature that contains the current item the
   * iterator is pointing to.
   *
   * If the iterator is pointing to a Feature, that Feature will be returned.
   */
  const GeoJsonObject* getFeature() { return this->_it.getFeature(); }

  /**
   * @brief Iterates to the next \ref GeoJsonObject, returning this modified
   * iterator.
   */
  ConstGeoJsonObjectIterator& operator++() {
    ++this->_it;
    return *this;
  }
  /**
   * @brief Iterates to the next \ref GeoJsonObject, returning the previous
   * state of the iterator.
   */
  ConstGeoJsonObjectIterator operator++(int) {
    ConstGeoJsonObjectIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  /**
   * @brief Checks if two \ref ConstGeoJsonObjectIterator iterators are equal.
   */
  friend bool operator==(
      const ConstGeoJsonObjectIterator& a,
      const ConstGeoJsonObjectIterator& b) {
    return a._it == b._it;
  }

  /**
   * @brief Checks if two \ref ConstGeoJsonObjectIterator iterators are not
   * equal.
   */
  friend bool operator!=(
      const ConstGeoJsonObjectIterator& a,
      const ConstGeoJsonObjectIterator& b) {
    return a._it != b._it;
  }

  /**
   * @brief Creates a new \ref ConstGeoJsonObjectIterator with the given \ref
   * GeoJsonObject as the root object.
   *
   * @param rootObject The root object of the new iterator. This will be the
   * first object returned.
   */
  ConstGeoJsonObjectIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {}
  /**
   * @brief Creates a new \ref ConstGeoJsonObjectIterator without any \ref
   * GeoJsonObject. This is equivalent to an "end" iterator.
   */
  ConstGeoJsonObjectIterator() = default;

private:
  GeoJsonObjectIterator _it;
};

/**
 * @brief Returns all geometry data of a given type from a \ref GeoJsonObject.
 *
 * @tparam SingleT The type of the "single" version of this geometry object. For
 * example, `Point`.
 * @tparam MultiT The type of the "multi" version of this geometry object. For
 * example, `MultiPoint`.
 * @tparam ValueT The type of the geometry data included in both
 * `SingleT::coordinates` and `MultiT::coordinates[i]`.
 */
template <typename SingleT, typename MultiT, typename ValueT>
struct ConstGeoJsonPrimitiveIterator {
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ValueT;
  using pointer = const ValueT*;
  using reference = const ValueT&;
  //! @endcond

  /**
   * @brief Returns a reference to the current value.
   */
  reference operator*() const {
    const GeoJsonMultiPoint* pMultiPoint =
        (*this->_it).template getIf<MultiT>();
    if (pMultiPoint) {
      return pMultiPoint->coordinates[this->_currentMultiIdx];
    }

    return (*this->_it).template get<SingleT>().coordinates;
  }
  /**
   * @brief Returns a pointer to the current value.
   */
  pointer operator->() { return &**this; }

  /**
   * @brief Iterates to the next value, returning the modified iterator.
   */
  ConstGeoJsonPrimitiveIterator& operator++() {
    this->iterate();
    return *this;
  }
  /**
   * @brief Iterates to the next value, returning the previous state of the
   * iterator.
   */
  ConstGeoJsonPrimitiveIterator operator++(int) {
    ConstGeoJsonPrimitiveIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  /**
   * @brief Checks if two \ref ConstGeoJsonPrimitiveIterator iterators are
   * equal.
   */
  friend bool operator==(
      const ConstGeoJsonPrimitiveIterator& a,
      const ConstGeoJsonPrimitiveIterator& b) {
    return a._it == b._it;
  }
  /**
   * @brief Checks if two \ref ConstGeoJsonPrimitiveIterator iterators are not
   * equal.
   */
  friend bool operator!=(
      const ConstGeoJsonPrimitiveIterator& a,
      const ConstGeoJsonPrimitiveIterator& b) {
    return a._it != b._it;
  }

  /**
   * @brief Creates a new \ref ConstGeoJsonPrimitiveIterator from the given root
   * \ref GeoJsonObject.
   */
  ConstGeoJsonPrimitiveIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {
    if (!_it.isEnded() && !(_it->template containsType<SingleT>() ||
                            _it->template containsType<MultiT>())) {
      this->iterate();
    }
  }

  /**
   * @brief Creates an empty \ref ConstGeoJsonPrimitiveIterator.
   */
  ConstGeoJsonPrimitiveIterator() = default;

private:
  void iterate() {
    if (!this->_it.isEnded() && this->_it->template containsType<MultiT>()) {
      const MultiT& multi = this->_it->template get<MultiT>();
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
             !(this->_it->template containsType<SingleT>() ||
               (this->_it->template containsType<MultiT>() &&
                !this->_it->template get<MultiT>().coordinates.empty())));
  }

  GeoJsonObjectIterator _it;
  size_t _currentMultiIdx = 0;
};

/**
 * @brief An iterator over all \ref GeoJsonObject objects that contain a value
 * of type `ObjectType`.
 */
template <typename ObjectType> struct ConstGeoJsonObjectTypeIterator {
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ObjectType;
  using pointer = const ObjectType*;
  using reference = const ObjectType&;
  //! @endcond

  /** @brief Returns a reference to the current object. */
  reference operator*() const {
    return (*this->_it).template get<ObjectType>().coordinates;
  }
  /** @brief Returns a pointer to the current object. */
  pointer operator->() { return &**this; }

  /**
   * @brief Iterates to the next `ObjectType`, returning this modified
   * iterator.
   */
  ConstGeoJsonObjectTypeIterator& operator++() {
    do {
      ++this->_it;
    } while (!this->_it.isEnded() &&
             this->_it->template containsType<ObjectType>());
    return *this;
  }
  /**
   * @brief Iterates to the next `ObjectType`, returning the previous
   * state of the iterator.
   */
  ConstGeoJsonObjectTypeIterator operator++(int) {
    ConstGeoJsonObjectTypeIterator tmp = *this;
    this->_it++;
    return tmp;
  }
  /**
   * @brief Checks if two \ref ConstGeoJsonObjectTypeIterator iterators are
   * equal.
   */
  friend bool operator==(
      const ConstGeoJsonObjectTypeIterator& a,
      const ConstGeoJsonObjectTypeIterator& b) {
    return a._it == b._it;
  }
  /**
   * @brief Checks if two \ref ConstGeoJsonObjectTypeIterator iterators are not
   * equal.
   */
  friend bool operator!=(
      const ConstGeoJsonObjectTypeIterator& a,
      const ConstGeoJsonObjectTypeIterator& b) {
    return a._it != b._it;
  }

  /**
   * @brief Creates a new \ref ConstGeoJsonObjectTypeIterator with the given
   * \ref GeoJsonObject as the root object.
   *
   * @param rootObject The root object of the new iterator. This will be the
   * first object returned.
   */
  ConstGeoJsonObjectTypeIterator(const GeoJsonObject& rootObject)
      : _it(const_cast<GeoJsonObject&>(rootObject)) {}
  /**
   * @brief Creates a new \ref ConstGeoJsonObjectTypeIterator without any \ref
   * GeoJsonObject. This is equivalent to an "end" iterator.
   */
  ConstGeoJsonObjectTypeIterator() = default;

private:
  GeoJsonObjectIterator _it;
  size_t _currentMultiPointIdx = 0;
};
} // namespace CesiumVectorData