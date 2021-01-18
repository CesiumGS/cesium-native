#pragma once

#include "CesiumGltf/ImageCesium.h"
#include "CesiumGltf/ImageSpec.h"

namespace CesiumGltf {
    /** @copydoc ImageSpec */
    struct Image : public ImageSpec {
        /**
         * @brief Holds properties that are specific to the glTF loader rather than part of the glTF spec.
         */
        ImageCesium cesium;
    };
}
