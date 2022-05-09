# This file doesn't do any useful building, it is just unit tests for functions
# in cesium.cmake

cmake_minimum_required(VERSION 3.15)

project(CesiumCMakeTests
    VERSION 0.1.0
    LANGUAGES CXX C
)

include(./cesium.cmake)

cesium_get_value_configurations("test;thing" TEST_OUT)
if (NOT "*" IN_LIST TEST_OUT)
  message("FAIL: cesium_get_value_configurations(\"test;thing\" TEST_OUT) != \"*\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("test;thing" "*" TEST_OUT)
if (NOT TEST_OUT STREQUAL "test;thing")
  message("FAIL: cesium_get_configuration_value(\"test;thing\" \"*\" TEST_OUT) != \"test;thing\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("test;thing" "Debug" TEST_OUT)
if (NOT TEST_OUT STREQUAL "test;thing")
  message("FAIL: cesium_get_configuration_value(\"test;thing\" \"Debug\" TEST_OUT) != \"test;thing\" instead \"${TEST_OUT}\"")
endif()

cesium_get_value_configurations("$<$<CONFIG:Debug>:test>;thing" TEST_OUT)
if (NOT "Debug" IN_LIST TEST_OUT OR NOT "*" IN_LIST TEST_OUT)
  message("FAIL: cesium_get_value_configurations(\"?<$<CONFIG:Debug>:test>;thing\" TEST_OUT) != \"Debug;*\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("$<$<CONFIG:Debug>:test>;thing" "Debug" TEST_OUT)
if (NOT TEST_OUT STREQUAL "test;thing")
  message("FAIL: cesium_get_configuration_value(\"$<$<CONFIG:Debug>:test>;thing\" \"Debug\" TEST_OUT) != \"test;thing\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("$<$<CONFIG:Debug>:test>;thing" "Release" TEST_OUT)
if (NOT TEST_OUT STREQUAL "thing")
  message("FAIL: cesium_get_configuration_value(\"$<$<CONFIG:Debug>:test>;thing\" \"Release\" TEST_OUT) != \"thing\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("foo;$<$<CONFIG:Debug>:test;this>$<$<CONFIG:Release>:release;stuff>;thing" "Release" TEST_OUT)
if (NOT TEST_OUT STREQUAL "foo;release;stuff;thing")
  message("FAIL: cesium_get_configuration_value(\"foo;$<$<CONFIG:Debug>:test;this>$<$<CONFIG:Release>:release;stuff>;thing\" \"Release\" TEST_OUT) != \"foo;release;stuff;thing\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("foo;$<$<CONFIG:Debug>:test;this>;$<$<CONFIG:Release>:release;stuff>" "Release" TEST_OUT)
if (NOT TEST_OUT STREQUAL "foo;release;stuff")
  message("FAIL: cesium_get_configuration_value(\"foo;$<$<CONFIG:Debug>:test;this>;$<$<CONFIG:Release>:release;stuff>\" \"Release\" TEST_OUT) != \"foo;release;stuff\" instead \"${TEST_OUT}\"")
endif()

cesium_get_configuration_value("foo;$<$<CONFIG:Debug>:test;this>;$<$<CONFIG:Release>:release;stuff>" "Debug" TEST_OUT)
if (NOT TEST_OUT STREQUAL "foo;test;this")
  message("FAIL: cesium_get_configuration_value(\"foo;$<$<CONFIG:Debug>:test;this>;$<$<CONFIG:Release>:release;stuff>\" \"Debug\" TEST_OUT) != \"foo;test;this\" instead \"${TEST_OUT}\"")
endif()
