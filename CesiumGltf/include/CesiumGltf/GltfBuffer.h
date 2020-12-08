#pragma once

#include <string>
#include <gsl/span>

struct cgltf_buffer;

namespace CesiumGltf {
    class GltfBuffer {
    public:
        static GltfBuffer createFromCollectionElement(cgltf_buffer* array, size_t arrayIndex);

        std::string getUri() const noexcept;
        void setUri(const std::string& value) noexcept;

        gsl::span<uint8_t> getData() noexcept;
        gsl::span<const uint8_t> getData() const noexcept;
        void setData(const gsl::span<const uint8_t>& value) noexcept;
        void resizeData(size_t newSize) noexcept;

    private:
        GltfBuffer(cgltf_buffer* p);

        cgltf_buffer* _p;
    };
}
