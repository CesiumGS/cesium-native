#pragma once

#include "CesiumAsync/IAssetResponse.h"
#include <vector>

class SimpleAssetResponse : public CesiumAsync::IAssetResponse {
public:
	SimpleAssetResponse(uint16_t statusCode,
		const std::string& contentType,
		const std::vector<uint8_t>& data)
		: mockStatusCode{ statusCode }
		, mockContentType{ contentType }
		, mockData{ data }
	{}

	virtual uint16_t statusCode() const override { return this->mockStatusCode; }

	virtual std::string contentType() const override { return this->mockContentType; }

	virtual gsl::span<const uint8_t> data() const override {
		return gsl::span<const uint8_t>(mockData.data(), mockData.size());
	}

	uint16_t mockStatusCode;
	std::string mockContentType;
	std::vector<uint8_t> mockData;
};
