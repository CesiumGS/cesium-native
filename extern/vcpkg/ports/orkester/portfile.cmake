# orkester vcpkg portfile
#
# Downloads pre-built orkester-ffi static library from a GitHub Release.
#
# The release tarball contains:
#   lib/orkester_ffi.lib (or .a)
#   include/orkester.h
#   lib/cmake/orkester/orkesterConfig.cmake

set(ORKESTER_VERSION "0.1.2")
set(ORKESTER_REPO "calebbuffa/socle")

# ── select the right tarball for this platform ───────────────────────────────

if(VCPKG_TARGET_IS_WINDOWS)
    set(ORKESTER_TRIPLET "x86_64-pc-windows-msvc")
    set(ORKESTER_SHA512 "3c68337b397059b719341babb50c941554b5382dc8e5c84bab259fd31229befaed6d7ca1ad590ba13b3217560a8e3dddf5a78114d7fdb8da47d149d8eaae91d6")
elseif(VCPKG_TARGET_IS_OSX)
    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
        set(ORKESTER_TRIPLET "aarch64-apple-darwin")
        set(ORKESTER_SHA512 "e359eac823a3794975f0946ec670116447b9130210f2e758151c13a1722fada39e5db0abc1bb91122cd7a2d882e8bac06b8e8a5e9e6478ad65aa86937611e3ca")
    else()
        set(ORKESTER_TRIPLET "x86_64-apple-darwin")
        set(ORKESTER_SHA512 "26a140ff4c2bae9eab3a9c708104fc4df874e1c77b97a0d4507064f9aa02e3cc631530989886b1fe7930263ced4c62fdace786e5c3cafabb880dd1ce1bdc4351")
    endif()
elseif(VCPKG_TARGET_IS_LINUX)
    set(ORKESTER_TRIPLET "x86_64-unknown-linux-gnu")
    set(ORKESTER_SHA512 "527423622be0704fd5507250db6128be31d7b72479bbb187c61c4c537542026970d1ddc35abe83ade67b174427391d9187c02c613edcbf1aa430f2811eb39630")
else()
    message(FATAL_ERROR "orkester: unsupported platform")
endif()

set(ORKESTER_URL
    "https://github.com/${ORKESTER_REPO}/releases/download/orkester/v${ORKESTER_VERSION}/orkester-${ORKESTER_TRIPLET}.tar.gz")

# ── download and extract ────────────────────────────────────────────────────

vcpkg_download_distfile(ARCHIVE
    URLS "${ORKESTER_URL}"
    FILENAME "orkester-${ORKESTER_TRIPLET}-${ORKESTER_VERSION}.tar.gz"
    SHA512 ${ORKESTER_SHA512}
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    NO_REMOVE_ONE_LEVEL
)

# ── install files to vcpkg directories ──────────────────────────────────────

# Headers
file(INSTALL "${SOURCE_PATH}/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

# Libraries
file(INSTALL "${SOURCE_PATH}/lib/" DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.a")

# Debug: same libraries (Rust release build works for both configs)
file(INSTALL "${SOURCE_PATH}/lib/" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib"
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.a")

# CMake config — install to original location, then let vcpkg relocate
file(INSTALL "${SOURCE_PATH}/lib/cmake/" DESTINATION "${CURRENT_PACKAGES_DIR}/lib/cmake")
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/orkester)

# License
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Apache-2.0 License")
