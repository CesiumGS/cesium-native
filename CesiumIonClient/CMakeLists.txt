add_library(CesiumIonClient "")

configure_cesium_library(CesiumIonClient)

file(GLOB SOURCES include/CesiumIonClient/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumIonClient/*.h)

target_sources(
    CesiumIonClient
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumIonClient
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/rapidjson/include

    PUBLIC
        include

        # Must set ${targetName}_PRIVATE_SRC to 1 to add src directory to the public include directory. 
        # This should only be set by the target itself and test target, and used for testing purpose only
        "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:CesiumIonClient_PRIVATE_SRC>>:${CMAKE_CURRENT_SOURCE_DIR}/src>>"
)

target_link_libraries(CesiumIonClient PUBLIC CesiumAsync CesiumUtility)
