#pragma once

#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/Assert.h>

#include <map>
#include <unordered_map>

namespace CesiumJsonReader {
/**
 * @brief Reads the keys and values of a JSON object into a
 * `std::map<std::string, T>` or an `std::unordered_map<std::string, T>`.
 *
 * @tparam T The type of values in the map.
 * @tparam THandler The type of the \ref IJsonHandler to handle the map values.
 */
template <typename T, typename THandler>
class CESIUMJSONREADER_API DictionaryJsonHandler : public ObjectJsonHandler {
public:
  /**
   * @brief Creates a new \ref DictionaryJsonHandler, passing the specified
   * arguments to the constructor of THandler.
   */
  template <typename... Ts>
  DictionaryJsonHandler(Ts&&... args) noexcept
      : ObjectJsonHandler(), _item(std::forward<Ts>(args)...) {}

  /**
   * @brief Resets the parent of this \ref IJsonHandler and sets the destination
   * pointer of this handler to an `std::unordered_map<std::string, T>`.
   *
   * @warning Technically, there is no reason why you can't call `reset` twice
   * on the same \ref DictionaryJsonHandler, once with an `std::map` and once
   * with an `std::unordered_map`. In practice, if a pointer to an
   * `std::unordered_map` is present, it will always be used as the destination
   * and the pointer to an `std::map` will be ignored.
   */
  void reset(
      IJsonHandler* pParent,
      std::unordered_map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary1 = pDictionary;
  }

  /**
   * @brief Resets the parent of this \ref IJsonHandler and sets the destination
   * pointer of this handler to an `std::map<std::string, T>`.
   */
  void reset(IJsonHandler* pParent, std::map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary2 = pDictionary;
  }

  /** @copydoc IJsonHandler::readObjectKey */
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override {
    CESIUM_ASSERT(this->_pDictionary1 || this->_pDictionary2);

    if (this->_pDictionary1) {
      auto it = this->_pDictionary1->emplace(str, T()).first;

      return this->property(it->first.c_str(), this->_item, it->second);
    }

    auto it = this->_pDictionary2->emplace(str, T()).first;

    return this->property(it->first.c_str(), this->_item, it->second);
  }

private:
  std::unordered_map<std::string, T>* _pDictionary1 = nullptr;
  std::map<std::string, T>* _pDictionary2 = nullptr;
  THandler _item;
};
} // namespace CesiumJsonReader
