vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kring/KTX-Software # KhronosGroup/KTX-Software
    REF "v${VERSION}"
    SHA512 fa20457fc0f0b117f4d6b406baa338091a85bcc46f60ca440dcd483388c67c550c81da3618d70d748e12850e32e0c1d82e9e8dc8522849074f40cc455723ac97
    HEAD_REF master
)
file(REMOVE "${SOURCE_PATH}/other_include/zstd_errors.h")
file(REMOVE_RECURSE "${SOURCE_PATH}/external/basisu/zstd")
file(REMOVE_RECURSE "${SOURCE_PATH}/lib/basisu/zstd")

vcpkg_list(SET OPTIONS)
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_acquire_msys(MSYS_ROOT
        PACKAGES
            bash
        DIRECT_PACKAGES
            # Required for "getopt"
            "https://mirror.msys2.org/msys/x86_64/util-linux-2.40.2-2-x86_64.pkg.tar.zst"
            bf45b16cd470f8d82a9fe03842a09da2e6c60393c11f4be0bab354655072c7a461afc015b9c07f9f5c87a0e382cd867e4f079ede0d42f1589aa99ebbb3f76309
            # Required for "dos2unix"
            "https://mirror.msys2.org/msys/x86_64/dos2unix-7.5.3-1-x86_64.pkg.tar.zst"
            ab5f88b10577b1d195d9b7b74c1a46d9e715c5fac21e8da3d590f345294190ed1ce7fde37d765f51ba01c2f6706c077123d4c69cfd0981729fdcb3d30f85bc6d
    )
    vcpkg_add_to_path("${MSYS_ROOT}/usr/bin")
    vcpkg_list(APPEND OPTIONS "-DBASH_EXECUTABLE=${MSYS_ROOT}/usr/bin/bash.exe")
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools   KTX_FEATURE_TOOLS
        vulkan  LIBKTX_FEATURE_VK_UPLOAD
        js      KTX_FEATURE_JS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DKTX_GIT_VERSION_FULL=v${VERSION}
        -DKTX_FEATURE_TESTS=OFF
        -DKTX_FEATURE_LOADTEST_APPS=OFF
        -DKTX_FEATURE_EMBEDDED_ZSTD=OFF
        -DKTX_FEATURE_EMBEDDED_TOOLS_DEPENDENCIES=OFF
        -DLIBKTX_FEATURE_APPLE_FRAMEWORK=OFF
        ${FEATURE_OPTIONS}
        ${OPTIONS}
    DISABLE_PARALLEL_CONFIGURE
)

vcpkg_cmake_install()

if(tools IN_LIST FEATURES)
    vcpkg_copy_tools(
        TOOL_NAMES
            ktx
            toktx
            ktxsc
            ktxinfo
            ktx2ktx2
            ktx2check
        AUTO_CLEAN
    )
else()
    vcpkg_copy_pdbs()
endif()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ktx)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(GLOB LICENSE_FILES "${SOURCE_PATH}/LICENSES/*")
file(COPY ${LICENSE_FILES} DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/LICENSES")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
