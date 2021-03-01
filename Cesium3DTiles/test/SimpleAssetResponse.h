#pragma once

#include "CesiumAsync/IAssetResponse.h"
#include <vector>

class SimpleAssetResponse : public CesiumAsync::IAssetResponse {
public:
	SimpleAssetResponse(uint16_t statusCode,
		const std::string& contentType,
        const CesiumAsync::HttpHeaders& headers, 
		const std::vector<uint8_t>& data)
		: mockStatusCode{ statusCode }
		, mockContentType{ contentType }
		, mockHeaders{ headers }
		, mockData{ data }
	{}

	virtual uint16_t statusCode() const override { return this->mockStatusCode; }

	virtual std::string contentType() const override { return this->mockContentType; }

    virtual const CesiumAsync::HttpHeaders& headers() const override { return this->mockHeaders; }

	virtual gsl::span<const uint8_t> data() const override {
		return gsl::span<const uint8_t>(mockData.data(), mockData.size());
	}

	uint16_t mockStatusCode;
	std::string mockContentType;
    CesiumAsync::HttpHeaders mockHeaders;
	std::vector<uint8_t> mockData;
};

