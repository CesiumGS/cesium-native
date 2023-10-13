#pragma once

#include "IPropertyTableViewVisitor.h"

namespace CesiumGltf {

template <typename Callback>
class GenericPropertyTableViewVisitor : public IPropertyTableViewVisitor {
private:
  Callback _callback;

public:
  GenericPropertyTableViewVisitor(const Callback& callback)
      : _callback(callback) {}
  GenericPropertyTableViewVisitor(Callback&& callback)
      : _callback(std::move(callback)) {}

  virtual void
  visit(const PropertyTablePropertyView<int8_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint8_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int16_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint16_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int32_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint32_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int64_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint64_t, false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<float>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<double>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<bool>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<std::string_view>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, float>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, float>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, float>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, float>>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<glm::mat<2, 2, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, float>>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<glm::mat<3, 3, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, float>>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<glm::mat<4, 4, double>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint8_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint16_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint32_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint64_t>, false>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<PropertyArrayView<float>>&
                         view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<PropertyArrayView<double>>&
                         view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<PropertyArrayView<bool>>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<std::string_view>>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<2, float>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<2, double>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<3, float>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<3, double>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<4, float>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<glm::vec<4, double>>>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<PropertyArrayView<glm::mat<2, 2, float>>>&
          view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<
        PropertyArrayView<glm::mat<2, 2, double>>>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<PropertyArrayView<glm::mat<3, 3, float>>>&
          view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<
        PropertyArrayView<glm::mat<3, 3, double>>>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint8_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint16_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint32_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint64_t>>,
                     false>& view) noexcept {
    _callback(view);
  }
  virtual void visit(
      const PropertyTablePropertyView<PropertyArrayView<glm::mat<4, 4, float>>>&
          view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<
        PropertyArrayView<glm::mat<4, 4, double>>>& view) noexcept {
    _callback(view);
  }

  virtual void
  visit(const PropertyTablePropertyView<int8_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint8_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int16_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint16_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int32_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint32_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<int64_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<uint64_t, true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<glm::vec<2, int8_t>, true>&
                         view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<2, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<glm::vec<3, int8_t>, true>&
                         view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<3, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<glm::vec<4, int8_t>, true>&
                         view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::vec<4, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<2, 2, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<3, 3, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<glm::mat<4, 4, uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint8_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint16_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint32_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<int64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void
  visit(const PropertyTablePropertyView<PropertyArrayView<uint64_t>, true>&
            view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<2, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<3, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::vec<4, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<2, 2, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<3, 3, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint8_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint16_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint32_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, int64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
  virtual void visit(const PropertyTablePropertyView<
                     PropertyArrayView<glm::mat<4, 4, uint64_t>>,
                     true>& view) noexcept {
    _callback(view);
  }
};

} // namespace CesiumGltf
