#pragma once

#include <rapidjson/document.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
namespace CesiumImpl {

class BatchTableHierarchyPropertyValues {
public:
  class const_iterator {
  public:
    const_iterator(
        const std::vector<const rapidjson::Value*>& propertyInClass,
        const rapidjson::Value::ConstArray& classIds,
        const rapidjson::Value::ConstArray& parentIds,
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
    const rapidjson::Value::ConstArray _classIds;
    const rapidjson::Value::ConstArray _parentIds;
    const std::vector<uint32_t>& _instanceIndices;
    int64_t _currentIndex;
    mutable const rapidjson::Value* _pCachedValue;
  };

  BatchTableHierarchyPropertyValues(
      const rapidjson::Value& batchTableHierarchy);

  void setProperty(const std::string& propertyName);

  const_iterator begin() const;
  const_iterator end() const;

  int64_t size() const;

private:
  const_iterator createIterator(int64_t index) const;

  const rapidjson::Value& _batchTableHierarchy;
  std::string _propertyName;

  // The index of each instance within its class.
  std::vector<uint32_t> _instanceIndices;

  // A pointer to the current property in each class.
  std::vector<const rapidjson::Value*> _indexOfPropertyInClass;
};

} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
