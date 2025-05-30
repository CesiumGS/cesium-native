#pragma once

#include "CesiumUtility/Result.h"

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace CesiumUtility;

struct UndefinedPlaceholder {};

struct ArrayPlaceholder;

using VectorStyleExpressionValue = std::variant<
    bool,
    std::nullptr_t,
    UndefinedPlaceholder,
    double,
    std::string,
    ArrayPlaceholder,
    glm::dvec2,
    glm::dvec3,
    glm::dvec4,
    std::regex>;

struct ArrayPlaceholder {
  std::vector<VectorStyleExpressionValue> values;
};

struct VectorStyleExpressionContext {
  rapidjson::Document feature;
};

/**
 * @brief Implementing the horrible JavaScript type conversion rules.
 */
class TypeConverter {
public:
  static bool toBoolean(const VectorStyleExpressionValue& value) {
    struct Visitor {
      bool operator()(const bool& v) { return v; }
      bool operator()(const std::nullptr_t&) { return false; }
      bool operator()(const UndefinedPlaceholder&) { return false; }
      bool operator()(const double& v) { return !std::isnan(v) && v != 0; }
      bool operator()(const std::string& v) { return !v.empty() && v != "0"; }
      bool operator()(const ArrayPlaceholder&) { return true; }
      bool operator()(const glm::dvec2&) { return true; }
      bool operator()(const glm::dvec3&) { return true; }
      bool operator()(const glm::dvec4&) { return true; }
      bool operator()(const std::regex&) { return true; }
    };

    return std::visit(Visitor{}, value);
  }

  static double toNumber(const VectorStyleExpressionValue& value) {

    struct Visitor {
      double operator()(const bool& v) { return v ? 1.0 : 0.0; }
      double operator()(const std::nullptr_t&) { return 0.0; }
      double operator()(const UndefinedPlaceholder&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const double& v) { return v; }
      double operator()(const std::string& v) {
        char* pEnd = nullptr;
        double d = std::strtod(v.c_str(), &pEnd);
        if (v.c_str() == pEnd) {
          return std::numeric_limits<double>::quiet_NaN();
        }

        return d;
      }
      double operator()(const ArrayPlaceholder& arr) {
        if (arr.values.size() == 0) {
          return 0.0;
        }

        if (arr.values.size() == 1) {
          const double* pValue = std::get_if<double>(&arr.values[0]);
          if (pValue) {
            return *pValue;
          }
        }

        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec2&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec3&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec4&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const std::regex&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
    };

    return std::visit(Visitor{}, value);
  }

  static std::string toString(const VectorStyleExpressionValue& value) {
    struct Visitor {
      std::string operator()(const bool& v) { return v ? "true" : "false"; }
      std::string operator()(const std::nullptr_t&) { return "null"; }
      std::string operator()(const UndefinedPlaceholder&) {
        return "undefined";
      }
      std::string operator()(const double& v) {
        if (std::isinf(v)) {
          return "Infinity";
        } else if (std::isnan(v)) {
          return "NaN";
        }

        return std::to_string(v);
      }
      std::string operator()(const std::string& v) { return v; }
      std::string operator()(const ArrayPlaceholder& arr) {
        std::string str;
        for (const VectorStyleExpressionValue& v : arr.values) {
          if (!str.empty()) {
            str += ",";
          }

          str += TypeConverter::toString(v);
        }
        return str;
      }
      std::string operator()(const glm::dvec2& v) {
        return fmt::format("({}, {})", v.x, v.y);
      }
      std::string operator()(const glm::dvec3& v) {
        return fmt::format("({}, {}, {})", v.x, v.y, v.z);
      }
      std::string operator()(const glm::dvec4& v) {
        return fmt::format("({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      }
      std::string operator()(const std::regex&) { return "RegExp"; }
    };

    return std::visit(Visitor{}, value);
  }

  static bool areExactlyEqual(
      const VectorStyleExpressionValue& value1,
      const VectorStyleExpressionValue& value2) {
    if (value1.index() != value2.index()) {
      // Different types, they can't be exactly equal
      return false;
    }

    struct Visitor {
      const VectorStyleExpressionValue& value2;
      bool operator()(const bool& v) { return v == std::get<bool>(value2); }
      bool operator()(const std::nullptr_t&) { return true; }
      bool operator()(const UndefinedPlaceholder&) { return true; }
      bool operator()(const double& v) { return v == std::get<double>(value2); }
      bool operator()(const std::string& v) {
        return v == std::get<std::string>(value2);
      }
      bool operator()(const ArrayPlaceholder&) { return false; }
      bool operator()(const glm::dvec2& v) {
        const glm::dvec2 v2 = std::get<glm::dvec2>(value2);
        return v.x == v2.x && v.y == v2.y;
      }
      bool operator()(const glm::dvec3& v) {
        const glm::dvec3 v2 = std::get<glm::dvec3>(value2);
        return v.x == v2.x && v.y == v2.y && v.z == v2.z;
      }
      bool operator()(const glm::dvec4& v) {
        const glm::dvec4 v2 = std::get<glm::dvec4>(value2);
        return v.x == v2.x && v.y == v2.y && v.z == v2.z && v.w == v2.w;
      }
      bool operator()(const std::regex&) { return false; }
    };

    return std::visit(Visitor{value2}, value1);
  }
};