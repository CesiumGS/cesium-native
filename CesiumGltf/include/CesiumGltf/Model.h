#pragma once

#include "CesiumGltf/Library.h"
#include "CesiumGltf/ModelSpec.h"

namespace CesiumGltf {
    /** @copydoc ModelSpec */
    struct CESIUMGLTF_API Model : public ModelSpec {
        /**
         * @brief Merges another model into this one.
         * 
         * After this method returns, this `Model` contains all of the
         * elements that were originally in it _plus_ all of the elements
         * that were in `rhs`. Element indices are updated accordingly.
         * However, element indices in {@link ExtensibleObject::extras}, if any,
         * are _not_ updated.
         * 
         * @param rhs The model to merge into this one.
         */
        void merge(Model&& rhs);

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
         * @param pItems The array.
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
         * @param pItems The array.
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
