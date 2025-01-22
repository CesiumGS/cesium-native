#include "BatchTableHierarchyPropertyValues.h"

#include <CesiumUtility/Assert.h>

#include <glm/common.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace Cesium3DTilesContent::CesiumImpl;

namespace {

rapidjson::Value createEmptyArray() {
  rapidjson::Value result;
  result.SetArray();
  return result;
}

} // namespace

BatchTableHierarchyPropertyValues::BatchTableHierarchyPropertyValues(
    const rapidjson::Value& batchTableHierarchy,
    int64_t batchLength)
    : _batchTableHierarchy(batchTableHierarchy),
      _batchLength(batchLength),
      _pClassIDs(nullptr),
      _pParentIDs(nullptr),
      _instanceIndices(),
      _propertyInClass() {
  static const rapidjson::Value emptyArray = createEmptyArray();

  auto classIdsIt = this->_batchTableHierarchy.FindMember("classIds");
  if (classIdsIt == this->_batchTableHierarchy.MemberEnd() ||
      !classIdsIt->value.IsArray()) {
    this->_pClassIDs = &emptyArray;
  } else {
    this->_pClassIDs = &classIdsIt->value;
  }

  auto parentIdsIt = this->_batchTableHierarchy.FindMember("parentIds");
  if (parentIdsIt == this->_batchTableHierarchy.MemberEnd() ||
      !parentIdsIt->value.IsArray()) {
    this->_pParentIDs = &emptyArray;
  } else {
    this->_pParentIDs = &parentIdsIt->value;
  }

  auto classesIt = batchTableHierarchy.FindMember("classes");
  if (classesIt == batchTableHierarchy.MemberEnd()) {
    return;
  }

  auto instancesLengthIt = batchTableHierarchy.FindMember("instancesLength");
  if (instancesLengthIt == batchTableHierarchy.MemberEnd() ||
      !instancesLengthIt->value.IsInt64()) {
    return;
  }

  int64_t instancesLength = instancesLengthIt->value.GetInt64();

  std::vector<uint32_t> classInstancesSeen(classesIt->value.Size(), 0);
  this->_instanceIndices.resize(size_t(instancesLength), 0);

  size_t instanceIndex = 0;
  for (const rapidjson::Value& classIdValue : classIdsIt->value.GetArray()) {
    if (!classIdValue.IsInt64()) {
      continue;
    }

    int64_t classId = classIdValue.GetInt64();
    if (classId < 0 || classId >= int64_t(classInstancesSeen.size())) {
      // Invalid class ID
      continue;
    }

    this->_instanceIndices[instanceIndex] = classInstancesSeen[size_t(classId)];
    ++classInstancesSeen[size_t(classId)];

    ++instanceIndex;

    if (instanceIndex >= this->_instanceIndices.size()) {
      // Shouldn't happen in a correctly-defined batch table hierarchy, but
      // don't overflow buffers if it does.
      break;
    }
  }
}

void BatchTableHierarchyPropertyValues::setProperty(
    const std::string& propertyName) {
  this->_propertyInClass.clear();

  auto classesIt = this->_batchTableHierarchy.FindMember("classes");
  if (classesIt == this->_batchTableHierarchy.MemberEnd() ||
      !classesIt->value.IsArray()) {
    return;
  }

  auto classes = classesIt->value.GetArray();

  rapidjson::Value propertyNameValue;
  propertyNameValue.SetString(
      propertyName.data(),
      rapidjson::SizeType(propertyName.size()));

  for (auto it = classes.Begin(); it != classes.End(); ++it) {
    auto instancesIt = it->FindMember("instances");
    if (instancesIt == it->MemberEnd()) {
      this->_propertyInClass.emplace_back(nullptr);
      continue;
    }

    auto propertyIt = instancesIt->value.FindMember(propertyNameValue);
    if (propertyIt == instancesIt->value.MemberEnd()) {
      this->_propertyInClass.emplace_back(nullptr);
      continue;
    }

    if (!propertyIt->value.IsArray()) {
      this->_propertyInClass.emplace_back(nullptr);
      continue;
    }

    this->_propertyInClass.emplace_back(&propertyIt->value);
  }
}

BatchTableHierarchyPropertyValues::const_iterator
BatchTableHierarchyPropertyValues::begin() const {
  return createIterator(0);
}

BatchTableHierarchyPropertyValues::const_iterator
BatchTableHierarchyPropertyValues::end() const {
  return createIterator(this->size());
}

int64_t BatchTableHierarchyPropertyValues::size() const {
  return glm::min(int64_t(this->_instanceIndices.size()), this->_batchLength);
}

BatchTableHierarchyPropertyValues::const_iterator
BatchTableHierarchyPropertyValues::createIterator(int64_t index) const {
  return const_iterator(
      this->_propertyInClass,
      *this->_pClassIDs,
      *this->_pParentIDs,
      this->_instanceIndices,
      index);
}

BatchTableHierarchyPropertyValues::const_iterator::const_iterator(
    const std::vector<const rapidjson::Value*>& propertyInClass,
    const rapidjson::Value& classIds,
    const rapidjson::Value& parentIds,
    const std::vector<uint32_t>& instanceIndices,
    int64_t currentIndex)
    : _propertyInClass(propertyInClass),
      _classIds(classIds),
      _parentIds(parentIds),
      _instanceIndices(instanceIndices),
      _currentIndex(currentIndex),
      _pCachedValue(nullptr) {}

BatchTableHierarchyPropertyValues::const_iterator&
BatchTableHierarchyPropertyValues::const_iterator::operator++() {
  this->_pCachedValue = nullptr;
  ++this->_currentIndex;
  return *this;
}

bool BatchTableHierarchyPropertyValues::const_iterator::operator==(
    const const_iterator& rhs) const {
  return this->_currentIndex == rhs._currentIndex;
}

bool BatchTableHierarchyPropertyValues::const_iterator::operator!=(
    const const_iterator& rhs) const {
  return this->_currentIndex != rhs._currentIndex;
}

const rapidjson::Value&
BatchTableHierarchyPropertyValues::const_iterator::operator*() const {
  // A static null value, to return when there is no value defined for this
  // instance and property.
  static const rapidjson::Value nullValue{};

  if (this->_pCachedValue) {
    return *this->_pCachedValue;
  }

  const rapidjson::Value* pResult = this->getValue(this->_currentIndex);
  if (pResult) {
    this->_pCachedValue = pResult;
    return *pResult;
  }

  int64_t id = this->_currentIndex;

  while (true) {
    if (id < 0 || id >= this->_parentIds.Size()) {
      this->_pCachedValue = &nullValue;
      return nullValue;
    }

    const rapidjson::Value& parentIdValue =
        this->_parentIds[rapidjson::SizeType(id)];
    if (!parentIdValue.IsInt64()) {
      this->_pCachedValue = &nullValue;
      return nullValue;
    }

    int64_t parentId = parentIdValue.GetInt64();
    if (parentId == id) {
      this->_pCachedValue = &nullValue;
      return nullValue;
    }

    pResult = this->getValue(parentId);
    if (pResult) {
      this->_pCachedValue = pResult;
      return *pResult;
    }

    id = parentId;
  }
}

const rapidjson::Value*
BatchTableHierarchyPropertyValues::const_iterator::operator->() const {
  return &(**this);
}

const rapidjson::Value*
BatchTableHierarchyPropertyValues::const_iterator::getValue(
    int64_t index) const {
  CESIUM_ASSERT(index < this->_classIds.Size());
  const rapidjson::Value& classIdValue =
      this->_classIds[rapidjson::SizeType(index)];
  if (!classIdValue.IsInt64()) {
    return nullptr;
  }

  int64_t classId = classIdValue.GetInt64();
  int64_t instanceId = this->_instanceIndices[size_t(index)];

  if (classId < 0 || classId >= int64_t(this->_propertyInClass.size())) {
    return nullptr;
  }

  const rapidjson::Value* pProperty = this->_propertyInClass[size_t(classId)];
  if (pProperty == nullptr) {
    return nullptr;
  }

  CESIUM_ASSERT(pProperty->IsArray());
  if (instanceId < 0 || instanceId >= pProperty->Size()) {
    return nullptr;
  }

  return &(*pProperty)[rapidjson::SizeType(instanceId)];
}
