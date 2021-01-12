#pragma once

#include "CesiumGltf/Accessor.h"
#include <cstdint>

namespace CesiumGltf {
    struct BufferView;

    /**
     * @brief Computes the number of components for a given accessor type.
     * 
     * For example `CesiumGltf::Accessor::Type::SCALAR` has 1 component while
     * `CesiumGltf::Accessor::Type::VEC4` has 4 components.
     * 
     * @param type The accessor type.
     * @return The number of components.
     */
    int8_t computeNumberOfComponents(CesiumGltf::Accessor::Type type);

    /**
     * @brief Returns the number of bytes for a given accessor component type.
     * 
     * For example `CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT` is 2 bytes while
     * `CesiumGltf::Accessor::ComponentType::FLOAT` is 4 bytes.
     * 
     * @param componentType The accessor component type.
     * @return The number of bytes for the component type.
     */
    int8_t computeByteSizeOfComponent(CesiumGltf::Accessor::ComponentType componentType);

    /**
     * @brief Computes the stride for a given {@link Accessor} and {@link BufferView}.
     * 
     * The stride is the number of bytes between the same elements of successive vertices.
     * 
     * @param accessor The accessor.
     * @param bufferView The buffer view.
     * @return The stride in bytes.
     */
    int64_t computeByteStride(const CesiumGltf::Accessor& accessor, const CesiumGltf::BufferView& bufferView);

    /**
     * @brief Safely gets the element with a given index, returning a default instance if the index is outside the range.
     * 
     * @tparam T The type of the array.
     * @param items The array.
     * @param index The index of the array element to retrieve.
     * @return The requested element, or a default instance if the index is invalid.
     */
    template <typename T>
    static const T& getSafe(const std::vector<T>& items, int32_t index) {
        static T default;
        if (index < 0 || static_cast<size_t>(index) >= items.size()) {
            return default;
        } else {
            return items[index];
        }
    }

    /**
     * @brief Safely gets a pointer to the element with a given index, returning `nullptr` if the index is outside the range.
     * 
     * @tparam T The type of the array.
     * @param items The array.
     * @param index The index of the array element to retrieve.
     * @return A pointer to the requested element, or `nullptr` if the index is invalid.
     */
    template <typename T>
    static const T* getSafe(const std::vector<T>* pItems, int32_t index) {
        if (index < 0 || static_cast<size_t>(index) >= pItems->size()) {
            return nullptr;
        } else {
            return &(*pItems)[index];
        }
    }

    /**
     * @brief Safely gets a pointer to the element with a given index, returning `nullptr` if the index is outside the range.
     * 
     * @tparam T The type of the array.
     * @param items The array.
     * @param index The index of the array element to retrieve.
     * @return A pointer to the requested element, or `nullptr` if the index is invalid.
     */
    template <typename T>
    static T* getSafe(std::vector<T>* pItems, int32_t index) {
        if (index < 0 || static_cast<size_t>(index) >= pItems->size()) {
            return nullptr;
        } else {
            return &(*pItems)[index];
        }
    }
}
