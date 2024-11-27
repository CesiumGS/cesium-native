if(NOT VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO groundswellaudio/swl-variant
    REF "f99acd71c9d9227de517b40684bd94544f2f8179"
    SHA512 6207d1388c0d9606d3e57d4f678281c172c67e82f6fe0f7dc0431d0fff3e57e9ac79238b44433745f8001d62c1d7209e264f4448d665396fca3753a933b4108b
    HEAD_REF main
    PATCHES
      "add-install.patch"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
