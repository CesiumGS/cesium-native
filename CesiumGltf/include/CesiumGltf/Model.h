#pragma once

#include "CesiumGltf/ModelSpec.h"

namespace CesiumGltf {
    /** @copydoc ModelSpec */
    struct Model : public ModelSpec {
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
            static T defaultObject;
            if (index < 0 || static_cast<size_t>(index) >= items.size()) {
                return defaultObject;
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
    };
}
