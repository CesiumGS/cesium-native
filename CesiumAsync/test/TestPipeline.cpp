#include "CesiumAsync/Pipeline.h"

#include <catch2/catch.hpp>

using namespace CesiumAsync;

TEST_CASE("Pipeline") {
  AsyncSystem async(nullptr);
  TestPipeline x(async, nullptr);
  x.run();
}
