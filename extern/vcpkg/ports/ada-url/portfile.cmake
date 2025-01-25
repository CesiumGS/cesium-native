vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ada-url/ada
    REF "920a8b34c194a4f3930c20b62afaee45f071ea37"
    SHA512 6e7d06cc9550fbe08984e835f7bbda4a322023e087425cf4e90a19ef019cedbe55df70cc6f61c21b771c7dae7ee1f56d3fcde3371206bac1adbe83dffa7cf032
    HEAD_REF main
    PATCHES
        #no-cpm.patch
)

vcpkg_check_features(
    OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools ADA_TOOLS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DADA_BENCHMARKS=OFF
        -DBUILD_TESTING=OFF
        -DCMAKE_DISABLE_FIND_PACKAGE_Python3=ON
        ${FEATURE_OPTIONS}
    OPTIONS_DEBUG
        -DADA_TOOLS=OFF
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(PACKAGE_NAME ada CONFIG_PATH "lib/cmake/ada")

if("tools" IN_LIST FEATURES)
    vcpkg_copy_tools(TOOL_NAMES adaparse AUTO_CLEAN)
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE-APACHE" "${SOURCE_PATH}/LICENSE-MIT")
