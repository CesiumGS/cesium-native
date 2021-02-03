#include "GltfTestUtils.h"

#include "CesiumGltf/Reader.h"
#include <iostream>

namespace CesiumGltfTest
{
	std::vector<std::byte> readFile(const std::string& filename)
	{
		std::vector<std::byte> vec;
		std::basic_ifstream<std::byte> file(filename.c_str(), std::ios::binary);
		if (!file) {
			return vec;
		}
		file.seekg(0, std::ios::end);
		std::ifstream::pos_type size = file.tellg();
		file.seekg(0, std::ios::beg);
		vec.resize(size_t(size));
		file.read(&vec[0], size);
		return vec;
	}

	DataReader createFileReader(const std::string& basePath)
	{
		DataReader dataReader = [&basePath](const std::string& relativePath) {
			std::string fullPath = basePath + relativePath;
			std::vector<std::byte> data = readFile(fullPath.c_str());
			return data;
		};
		return dataReader;
	}

	std::vector<SampleModel> readSampleModels(const DataReader& dataReader)
	{
		std::vector<SampleModel> sampleModels;

		std::vector<std::byte> indexFileData = dataReader("model-index.json");
		rapidjson::Document document;
		document.Parse(reinterpret_cast<const char*>(indexFileData.data()), indexFileData.size());
		if (document.HasParseError()) {
			std::cerr << "Could not read model-index.json" << std::endl;
			return sampleModels;
		}
		if (!document.IsArray()) {
			std::cerr << "Index file result is not an array" << std::endl;
			return sampleModels;
		}
		for (rapidjson::SizeType i = 0; i < document.Size(); i++) {
			const rapidjson::Value& entry = document[i];

			std::optional sampleModel = processSampleModel(i, entry);
			if (sampleModel.has_value()) {
				sampleModels.push_back(sampleModel.value());
			}
		}
		return sampleModels;
	}

	std::optional<SampleModel> processSampleModel(rapidjson::SizeType index, const rapidjson::Value& entry)
	{
		if (!entry.IsObject()) {
			std::cerr << "Entry " << index << " is not an object" << std::endl;
			return std::nullopt;
		}

		if (!entry.HasMember("name")) {
			std::cerr << "Entry " << index << " has no name" << std::endl;
			return std::nullopt;
		}
		const rapidjson::Value& nameValue = entry["name"];
		if (!nameValue.IsString()) {
			std::cerr << "Entry " << index << " does not have a name string" << std::endl;
			return std::nullopt;
		}
		std::string name = entry["name"].GetString();

		if (!entry.HasMember("variants")) {
			std::cerr << "Entry " << index << " has no variants" << std::endl;
			return std::nullopt;
		}
		const rapidjson::Value& variants = entry["variants"];
		if (!variants.IsObject()) {
			std::cerr << "Entry " << index << " has no valid variants" << std::endl;
			return std::nullopt;
		}

		std::vector<SampleModelVariant> sampleModelVariants = processVariants(name, variants);
		return std::optional<SampleModel>({ name, sampleModelVariants });
	}

	std::vector<SampleModelVariant> processVariants(const std::string& name, const rapidjson::Value& variants)
	{
		std::vector<SampleModelVariant> sampleModelVariants;
		for (rapidjson::Value::ConstMemberIterator itr = variants.MemberBegin(); itr != variants.MemberEnd(); ++itr) {
			std::string variantName = itr->name.GetString();
			const rapidjson::Value& variantFileNameValue = itr->value;
			if (!variantFileNameValue.IsString()) {
				std::cerr << "Variant in " << name << " does not have a valid name" << std::endl;
				continue;
			}
			std::string variantFileName = variantFileNameValue.GetString();
			sampleModelVariants.push_back({ variantName, variantFileName });
		}
		return sampleModelVariants;
	}


	bool testReadModel(const DataReader& dataReader, const std::string& name, const std::string& variantName, const std::string& variantFileName)
	{
		std::string subPath = name + "/" + variantName + "/" + variantFileName;
		std::vector<std::byte> data = dataReader(subPath);
		try
		{
			const uint8_t *dataBegin = reinterpret_cast<const uint8_t*>(data.data());
			CesiumGltf::ModelReaderResult result = CesiumGltf::readModel(gsl::span(dataBegin, data.size()));
			bool success = result.model.has_value();
			std::cout << "model " << name << " variant " << variantName << " " << (success ? "PASSED" : " !!! FAILED !!! ") << std::endl;
			return success;
		}
		catch (...)
		{
			std::cout << "model " << name << " variant " << variantName << " caused an error and !!! FAILED !!! " << std::endl;
			return false;
		}
	}

	bool testReadSampleModels(const std::string& basePath)
	{
		bool allPassed = true;

		std::vector<std::string> variantsToTest { "glTF", "glTF-Binary", "glTF-Embedded", "glTF-Draco" };

		DataReader dataReader = createFileReader(basePath);
		std::vector<SampleModel> sampleModels = readSampleModels(dataReader);
		for(const auto& sampleModel : sampleModels) {

			const std::string& name = sampleModel.name;
			//std::cout << "Processing " << name << std::endl;

			const auto& variants = sampleModel.sampleModelVariants;

			for(const auto& variant : variants) {

				const std::string& variantName = variant.variantName;
				const std::string& variantFileName = variant.variantFileName;
				if (std::find(variantsToTest.begin(), variantsToTest.end(), variantName) != variantsToTest.end()) {

					//std::cout << "  Testing model " << name << " variant " << variantName << std::endl;
					allPassed &= testReadModel(dataReader, name, variantName, variantFileName);
				}
			}
		}
		return allPassed;
	}
}

