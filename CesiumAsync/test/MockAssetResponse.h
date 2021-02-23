#pragma once

#include "CesiumAsync/IAssetResponse.h"
#include <vector>

class MockAssetResponse : public CesiumAsync::IAssetResponse {
public:
    MockAssetResponse(uint16_t statusCode, 
        const std::string& contentType, 
        const CesiumAsync::HttpHeaders& headers, 
        const std::vector<uint8_t>& data)
        : _statusCode{statusCode}
        , _contentType{contentType}
        , _headers{headers}
        , _data{data}
    {}

    virtual uint16_t statusCode() const override { return this->_statusCode; }

    virtual std::string contentType() const override { return this->_contentType; }

    virtual const CesiumAsync::HttpHeaders& headers() const override { return this->_headers; }

    virtual gsl::span<const uint8_t> data() const override {
        return gsl::span<const uint8_t>(_data.data(), _data.size());
    }

private:
    uint16_t _statusCode;
    std::string _contentType;
    CesiumAsync::HttpHeaders _headers;
    std::vector<uint8_t> _data;
};

