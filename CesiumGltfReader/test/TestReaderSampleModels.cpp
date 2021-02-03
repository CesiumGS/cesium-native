#include "catch2/catch.hpp"

#include "CesiumGltf/Reader.h"
#include "GltfTestUtils.h"

TEST_CASE("Read glTF Sample Models") {

    std::string basePath = "../CesiumGltfReader/test/data/2.0/";
    //basePath = "C:/Develop/KhronosGroup/glTF-Sample-Models/2.0/";
    CHECK(CesiumGltfTest::testReadSampleModels(basePath));
}
