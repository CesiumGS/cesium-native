#pragma once

#include "CesiumAsync/IAssetResponse.h"
#include <vector>

class MockAssetResponse : public CesiumAsync::IAssetResponse {
public:
    MockAssetResponse(uint16_t statusCode, 
        const std::string& contentType, 
        const CesiumAsync::HttpHeaders& headers, 
        std::optional<CesiumAsync::ResponseCacheControl> cacheControl,
        const std::vector<uint8_t>& data)
        : _statusCode{statusCode}
        , _contentType{contentType}
        , _headers{headers}
        , _cacheControl{std::move(cacheControl)}
        , _data{data}
    {}

    virtual uint16_t statusCode() const override { return this->_statusCode; }

    virtual const std::string& contentType() const override { return this->_contentType; }

    virtual const CesiumAsync::HttpHeaders& headers() const override { return this->_headers; }

    virtual const CesiumAsync::ResponseCacheControl* cacheControl() const override { 
        return this->_cacheControl ? &this->_cacheControl.value() : nullptr; 
    }

    virtual gsl::span<const uint8_t> data() const override {
        return gsl::span<const uint8_t>(_data.data(), _data.size());
    }

private:
    uint16_t _statusCode;
    std::string _contentType;
    CesiumAsync::HttpHeaders _headers;
    std::optional<CesiumAsync::ResponseCacheControl> _cacheControl;
    std::vector<uint8_t> _data;
};

