#pragma once

#include <rapidjson/document.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
namespace CesiumImpl {

/**
 * @brief An adaptor that flattens a batch table hierarchy, making it appear as
 * a flat list of instances with a flat list of properties.
 *
 * @private
 */
class BatchTableHierarchyPropertyValues {
public:
  class const_iterator {
  public:
    const_iterator(
        const std::vector<const rapidjson::Value*>& propertyInClass,
        const rapidjson::Value& classIds,
        const rapidjson::Value& parentIds,
        const std::vector<uint32_t>& instanceIndices,
        int64_t currentIndex);

    const_iterator& operator++();

    bool operator==(const const_iterator& rhs) const;
    bool operator!=(const const_iterator& rhs) const;

    const rapidjson::Value& operator*() const;
    const rapidjson::Value* operator->() const;

  private:
    const rapidjson::Value* getValue(int64_t index) const;

    const std::vector<const rapidjson::Value*>& _propertyInClass;
    const rapidjson::Value& _classIds;
    const rapidjson::Value& _parentIds;
    const std::vector<uint32_t>& _instanceIndices;
    int64_t _currentIndex;
    mutable const rapidjson::Value* _pCachedValue;
  };

  /**
   * @brief Constructs a new instance from a 3DTILES_batch_table_hierarchy JSON
   * value.
   *
   * @param batchTableHierarchy The 3DTILES_batch_table_hierarchy JSON value.
   * This value must remain valid for the entire lifetime of the
   * BatchTableHierarchyPropertyValues instance, or undefined behavior will
   * result.
   * @param batchLength The number of features, which may be less than the
   * number of instances in the batch table hierarchy.
   */
  BatchTableHierarchyPropertyValues(
      const rapidjson::Value& batchTableHierarchy,
      int64_t batchLength);

  /**
   * @brief Sets the name of the property whose values are to be enumerated.
   *
   * It is more efficient to re-use an instance to access different properties
   * than to create a new instance per property.
   *
   * @param propertyName The property name.
   */
  void setProperty(const std::string& propertyName);

  /**
   * @brief Gets an iterator for the value of this property for this first
   * feature.
   */
  const_iterator begin() const;

  /**
   * @brief Gets an iterator just past the last feature.
   */
  const_iterator end() const;

  /**
   * @brief Gets the total number of features.
   *
   * This is the smaller of the number of features (given to the constructor as
   * `batchLength`) and the number of instances in the batch table hierarchy.
   */
  int64_t size() const;

private:
  const_iterator createIterator(int64_t index) const;

  const rapidjson::Value& _batchTableHierarchy;
  int64_t _batchLength;
  const rapidjson::Value* _pClassIDs;
  const rapidjson::Value* _pParentIDs;

  // The index of each instance within its class.
  std::vector<uint32_t> _instanceIndices;

  // A pointer to the current property in each class.
  std::vector<const rapidjson::Value*> _propertyInClass;
};

} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
