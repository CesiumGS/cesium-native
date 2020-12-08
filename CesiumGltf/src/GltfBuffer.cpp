#include "CesiumGltf/GltfBuffer.h"
#include <cgltf.h>
#include <algorithm>

namespace CesiumGltf {
    /*static*/ GltfBuffer GltfBuffer::createFromCollectionElement(cgltf_buffer* array, size_t arrayIndex) {
        return GltfBuffer(&array[arrayIndex]);
    }

    GltfBuffer::GltfBuffer(cgltf_buffer* p) :
        _p(p)
    {
    }

    std::string GltfBuffer::getUri() const noexcept {
        return this->_p->uri;
    }

    void GltfBuffer::setUri(const std::string& value) noexcept {
        this->_p->uri = static_cast<char*>(realloc(this->_p->uri, value.size() + 1));
        value.copy(this->_p->uri, value.size(), 0);
        this->_p->uri[value.size()] = '\0';
    }

    gsl::span<const uint8_t> GltfBuffer::getData() const noexcept {
        return gsl::span(static_cast<const uint8_t*>(this->_p->data), this->_p->size);
    }

    gsl::span<uint8_t> GltfBuffer::getData() noexcept {
        return gsl::span(static_cast<uint8_t*>(this->_p->data), this->_p->size);
    }

    void GltfBuffer::setData(const gsl::span<const uint8_t>& value) noexcept {
        this->_p->data = realloc(this->_p->data, value.size());
        std::copy(value.begin(), value.end(), static_cast<uint8_t*>(this->_p->data));
    }

    void GltfBuffer::resizeData(size_t newSize) noexcept {
        this->_p->data = realloc(this->_p->data, newSize);
    }

}
