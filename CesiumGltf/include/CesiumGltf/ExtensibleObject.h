#pragma once

#include <any>
#include <vector>

namespace CesiumGltf {
    /**
     * @brief The base class for objects in a glTF that have extensions and extras.
     */
    struct ExtensibleObject {
        // TODO: extras

        /**
         * @brief Gets an extension given its static type.
         * 
         * @tparam T The type of the extension.
         * @return A pointer to the extension, or nullptr if the extension is not attached to this object.
         */
        template <typename T>
        const T* getExtension() const noexcept {
            for (const std::any& extension : extensions) {
                const T* p = std::any_cast<T>(&extension);
                if (p) {
                    return p;
                }
            }
            return nullptr;
        }

        /** @copydoc ExtensionObject::getExtension */
        template <typename T>
        T* getExtension() noexcept {
            return const_cast<T*>(this->getExtension<T>());
        }

        /**
         * @brief The extensions attached to this object.
         * 
         * Use {@link getExtension} to get the extension with a particular static type.
         */
        std::vector<std::any> extensions;
    };
}
