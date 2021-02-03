#pragma once
 
#include <rapidjson/document.h>

#include <vector>
#include <fstream>
#include <optional>
#include <functional>

namespace CesiumGltfTest
{
	struct SampleModelVariant
	{
		std::string variantName;
		std::string variantFileName;
	};

	struct SampleModel
	{
		std::string name;
		std::vector<SampleModelVariant> sampleModelVariants;
	};

	typedef std::function<std::vector<std::byte>(const std::string& relativePath)> DataReader;

	std::vector<std::byte> readFile(const std::string& filename);

	DataReader createFileReader(const std::string& basePath);

	std::vector<SampleModel> readSampleModels(const DataReader& dataReader);
	std::optional<SampleModel> processSampleModel(rapidjson::SizeType index, const rapidjson::Value& entry);
	std::vector<SampleModelVariant> processVariants(const std::string& name, const rapidjson::Value& variants);

	bool testReadModel(const DataReader& dataReader, const std::string& name, const std::string& variantName, const std::string& variantFileName);

	bool testReadSampleModels(const std::string& basePath);
}

