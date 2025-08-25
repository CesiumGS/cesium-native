#pragma once

#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorStyle.h>

#include <array>
#include <vector>

namespace CesiumVectorData {

struct GeoJsonObjectIterator;
struct ConstGeoJsonObjectIterator;
template <typename TSingle, typename TMulti, typename TValue>
struct ConstGeoJsonPrimitiveIterator;
template <typename TObject> struct ConstGeoJsonObjectTypeIterator;

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
 * @brief An object in a GeoJSON document.
 */
struct GeoJsonObject {
  /**
   * @brief An object providing `begin` and `end` methods for creating iterators
   * of the given type for a \ref GeoJsonObject.
   */
  template <typename TIterator> struct IteratorProvider {
    /**
     * @brief Returns an iterator pointing to the first element.
     */
    TIterator begin() { return TIterator(*this->_pObject); }

    /**
     * @brief Returns an iterator pointing "past the end" of all the elements.
     */
    TIterator end() { return TIterator(); }

  private:
    IteratorProvider(const GeoJsonObject* pObject) : _pObject(pObject) {}

    const GeoJsonObject* _pObject;
    friend struct GeoJsonObject;
  };

  /**
   * @brief Returns an iterator pointing to this object. Iterating this will
   * provide all children of this object.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  GeoJsonObjectIterator begin();
  /**
   * @brief Returns an iterator pointing "past the end" of the list of children
   * of this object.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  GeoJsonObjectIterator end();

  /**
   * @brief Returns an iterator pointing to this object. Iterating this will
   * provide all children of this object.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  ConstGeoJsonObjectIterator begin() const;
  /**
   * @brief Returns an iterator pointing "past the end" of the list of children
   * of this object.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  ConstGeoJsonObjectIterator end() const;

  /**
   * @brief Allows iterating over all points defined in this object or any child
   * objects. This will include both `Point` objects and `MultiPoint` objects.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  IteratorProvider<ConstGeoJsonPointIterator> points() const;

  /**
   * @brief Allows iterating over all lines defined in this object or any child
   * objects. This will include both `LineString` objects and `MultiLineString`
   * objects.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  IteratorProvider<ConstGeoJsonLineStringIterator> lines() const;

  /**
   * @brief Allows iterating over all polygons defined in this object or any
   * child objects. This will include both `Polygon` objects and `MultiPolygon`
   * objects.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  IteratorProvider<ConstGeoJsonPolygonIterator> polygons() const;

  /**
   * @brief Returns all \ref GeoJsonObject values matching the given type in
   * this object or in any children.
   *
   * @note The iterator will only descend up to a depth of eight, which should
   * cover almost all GeoJSON documents.
   */
  template <typename TObject>
  IteratorProvider<ConstGeoJsonObjectTypeIterator<TObject>> allOfType() const {
    return IteratorProvider<ConstGeoJsonObjectTypeIterator<TObject>>(this);
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
   * @brief Returns the style set on this GeoJSON object, if any.
   */
  const std::optional<VectorStyle>& getStyle() const;

  /**
   * @brief Returns the style set on this GeoJSON object, if any.
   */
  std::optional<VectorStyle>& getStyle();

  /**
   * @brief Returns the \ref GeoJsonObjectType that this \ref GeoJsonObject
   * is wrapping.
   */
  GeoJsonObjectType getType() const;

  /**
   * @brief Returns whether the `value` of this \ref GeoJsonObject is of the
   * given type.
   */
  template <typename T> bool isType() const {
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
  template <typename Visitor> decltype(auto) visit(Visitor&& visitor) {
    return std::visit(std::forward<Visitor>(visitor), this->value);
  }

  /**
   * @brief Applies the visitor `visitor` to the value variant.
   */
  template <typename Visitor> decltype(auto) visit(Visitor&& visitor) const {
    return std::visit(std::forward<Visitor>(visitor), this->value);
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
private:
  static constexpr size_t StackSize = 8;
  struct IteratorStackState {
    GeoJsonObject* pObject;
    int64_t nextPos;
  };

public:
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
    for (int64_t i = this->_stackPos; i >= 0; i--) {
      if (this->_stack[(size_t)i].pObject->isType<GeoJsonFeature>()) {
        return this->_stack[(size_t)i].pObject;
      }
    }

    return nullptr;
  }

  /**
   * @brief Returns `true` if this is an "end" iterator (points past the end of
   * all objects).
   */
  bool isEnded() const {
    return this->_stackPos == -1 && this->_pCurrentObject == nullptr;
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
    if (a._pCurrentObject != b._pCurrentObject || a._stackPos != b._stackPos) {
      return false;
    }

    if (a._stackPos >= 0 && b._stackPos >= 0) {
      if (a._stack[(size_t)a._stackPos].nextPos !=
          b._stack[(size_t)b._stackPos].nextPos) {
        return false;
      }
      if (a._stack[(size_t)a._stackPos].pObject !=
          b._stack[(size_t)b._stackPos].pObject) {
        return false;
      }
    }

    return true;
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
      : _stackPos(0), _pCurrentObject(nullptr) {
    this->_stack[0].pObject = &rootObject;
    this->_stack[0].nextPos = -1;
    this->iterate();
  }

  /**
   * @brief Creates a new \ref GeoJsonObjectIterator without any \ref
   * GeoJsonObject. This is equivalent to an "end" iterator.
   */
  GeoJsonObjectIterator() : _stackPos(-1), _pCurrentObject(nullptr) {}

private:
  void handleChild(GeoJsonObject& child) {
    this->_pCurrentObject = &child;

    if ((this->_stackPos - 1) <= (int64_t)StackSize &&
        (child.isType<GeoJsonGeometryCollection>() ||
         child.isType<GeoJsonFeatureCollection>() ||
         child.isType<GeoJsonFeature>())) {

      ++this->_stackPos;
      this->_stack[(size_t)this->_stackPos].pObject = &child;
      this->_stack[(size_t)this->_stackPos].nextPos = 0;
    }
  }

  void iterate() {
    this->_pCurrentObject = nullptr;
    while (this->_stackPos >= 0 && this->_stackPos < (int64_t)StackSize &&
           this->_pCurrentObject == nullptr) {
      IteratorStackState& stackState = this->_stack[(size_t)this->_stackPos];
      GeoJsonObject* pNext = stackState.pObject;
      if (stackState.nextPos == -1) {
        this->_pCurrentObject = pNext;
        ++stackState.nextPos;
        continue;
      } else if (
          GeoJsonGeometryCollection* pCollection =
              std::get_if<GeoJsonGeometryCollection>(&pNext->value)) {
        if ((size_t)stackState.nextPos >= pCollection->geometries.size()) {
          // No children left
          --this->_stackPos;
          continue;
        }

        GeoJsonObject& child =
            pCollection->geometries[(size_t)stackState.nextPos];
        ++stackState.nextPos;
        this->handleChild(child);
        continue;
      } else if (
          GeoJsonFeatureCollection* pFeatureCollection =
              std::get_if<GeoJsonFeatureCollection>(&pNext->value)) {
        if ((size_t)stackState.nextPos >= pFeatureCollection->features.size()) {
          // No children left
          --this->_stackPos;
          continue;
        }

        GeoJsonObject& child =
            pFeatureCollection->features[(size_t)stackState.nextPos];
        ++stackState.nextPos;
        this->handleChild(child);
        continue;
      } else if (
          GeoJsonFeature* pFeature =
              std::get_if<GeoJsonFeature>(&pNext->value)) {
        const size_t expectedSize = pFeature->geometry == nullptr ? 0 : 1;
        if ((size_t)stackState.nextPos >= expectedSize) {
          // Feature only has zero or one child
          --this->_stackPos;
          continue;
        }

        ++stackState.nextPos;
        this->handleChild(*pFeature->geometry);
        continue;
      } else {
        // This object was a dud, try another
        --this->_stackPos;
      }
    }
  }

  std::array<IteratorStackState, StackSize> _stack;
  int64_t _stackPos = -1;
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
   * @brief Returns `true` if this is an "end" iterator (points past the end of
   * all objects).
   */
  bool isEnded() const { return this->_it.isEnded(); }

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
    ++this->_it;
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
 * @tparam TSingle The type of the "single" version of this geometry object. For
 * example, `Point`.
 * @tparam TMulti The type of the "multi" version of this geometry object. For
 * example, `MultiPoint`.
 * @tparam TValue The type of the geometry data included in both
 * `TSingle::coordinates` and `TMulti::coordinates[i]`.
 */
template <typename TSingle, typename TMulti, typename TValue>
struct ConstGeoJsonPrimitiveIterator {
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = TValue;
  using pointer = const TValue*;
  using reference = const TValue&;
  //! @endcond

  /**
   * @brief Returns a reference to the current value.
   */
  reference operator*() const {
    const TMulti* pMultiPoint = (*this->_it).template getIf<TMulti>();
    if (pMultiPoint) {
      return pMultiPoint->coordinates[this->_currentMultiIdx];
    }

    return (*this->_it).template get<TSingle>().coordinates;
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
    this->iterate();
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
    if (!_it.isEnded() &&
        !(_it->template isType<TSingle>() || _it->template isType<TMulti>())) {
      this->iterate();
    }
  }

  /**
   * @brief Creates an empty \ref ConstGeoJsonPrimitiveIterator.
   */
  ConstGeoJsonPrimitiveIterator() = default;

private:
  void iterate() {
    if (!this->_it.isEnded() && this->_it->template isType<TMulti>()) {
      const TMulti& multi = this->_it->template get<TMulti>();
      if ((int64_t)this->_currentMultiIdx <
          ((int64_t)multi.coordinates.size() - 1)) {
        ++this->_currentMultiIdx;
        return;
      }
    }

    this->_currentMultiIdx = 0;
    do {
      ++this->_it;
    } while (!this->_it.isEnded() &&
             !(this->_it->template isType<TSingle>() ||
               (this->_it->template isType<TMulti>() &&
                !this->_it->template get<TMulti>().coordinates.empty())));
  }

  GeoJsonObjectIterator _it;
  size_t _currentMultiIdx = 0;
};

/**
 * @brief An iterator over all \ref GeoJsonObject objects that contain a value
 * of type `ObjectType`.
 */
template <typename TObject> struct ConstGeoJsonObjectTypeIterator {
  // Ignore Doxygen warnings for iterator tags.
  //! @cond Doxygen_Suppress
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = TObject;
  using pointer = const TObject*;
  using reference = const TObject&;
  //! @endcond

  /** @brief Returns a reference to the current object. */
  reference operator*() const { return (*this->_it).template get<TObject>(); }
  /** @brief Returns a pointer to the current object. */
  pointer operator->() { return &**this; }

  /**
   * @brief Iterates to the next `ObjectType`, returning this modified
   * iterator.
   */
  ConstGeoJsonObjectTypeIterator& operator++() {
    this->iterate();
    return *this;
  }
  /**
   * @brief Iterates to the next `ObjectType`, returning the previous
   * state of the iterator.
   */
  ConstGeoJsonObjectTypeIterator operator++(int) {
    ConstGeoJsonObjectTypeIterator tmp = *this;
    this->iterate();
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
      : _it(const_cast<GeoJsonObject&>(rootObject)) {
    if (!this->_it.isEnded() && !this->_it->template isType<TObject>()) {
      this->iterate();
    }
  }
  /**
   * @brief Creates a new \ref ConstGeoJsonObjectTypeIterator without any \ref
   * GeoJsonObject. This is equivalent to an "end" iterator.
   */
  ConstGeoJsonObjectTypeIterator() = default;

private:
  void iterate() {
    do {
      ++this->_it;
    } while (!this->_it.isEnded() && !this->_it->template isType<TObject>());
  }

  ConstGeoJsonObjectIterator _it;
  size_t _currentMultiPointIdx = 0;
};
} // namespace CesiumVectorData