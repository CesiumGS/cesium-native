#include "CesiumUtility/Gunzip.h"

#include <catch2/catch.hpp>
#include <algorithm>

using namespace CesiumUtility;

// "hello world!" gzipped, without file name or date info
static std::byte testZippedArray[] = {
    std::byte{0x1f}, std::byte{0x8b}, std::byte{0x08}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x0b}, std::byte{0xcb}, std::byte{0x48},
    std::byte{0xcd}, std::byte{0xc9}, std::byte{0xc9}, std::byte{0x57},
    std::byte{0x28}, std::byte{0xcf}, std::byte{0x2f}, std::byte{0xca},
    std::byte{0x49}, std::byte{0x51}, std::byte{0x04}, std::byte{0x00},
    std::byte{0x6d}, std::byte{0xc2}, std::byte{0xb4}, std::byte{0x03},
    std::byte{0x0c}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}};
static const gsl::span<std::byte> testZipped(testZippedArray, 32);

// "hello world!" in ASCII
static std::byte testUnzippedArray[] = {
    std::byte{0x68},
    std::byte{0x65},
    std::byte{0x6c},
    std::byte{0x6c},
    std::byte{0x6f},
    std::byte{0x20},
    std::byte{0x77},
    std::byte{0x6f},
    std::byte{0x72},
    std::byte{0x6c},
    std::byte{0x64},
    std::byte{0x21}};
static const gsl::span<std::byte> testUnzipped(testUnzippedArray, 12);

TEST_CASE("CesiumUtility::isGzip") {
  CHECK(CesiumUtility::isGzip(testZipped));
  CHECK(!CesiumUtility::isGzip(testUnzipped));
}

TEST_CASE("CesiumUtility::gunzip") {
  std::vector<std::byte> outVector;
  CHECK(CesiumUtility::gunzip(testZipped, outVector));
  CHECK(outVector.size() == testUnzipped.size());
  CHECK(std::equal(outVector.begin(), outVector.end(), testUnzipped.begin()));
  CHECK(!CesiumUtility::gunzip(testUnzipped, outVector));
}
