# orkester vcpkg portfile
#
# Downloads pre-built orkester-ffi static library from a GitHub Release.
#
# The release tarball contains:
#   lib/orkester_ffi.lib (or .a)
#   include/orkester.h
#   lib/cmake/orkester/orkesterConfig.cmake

set(ORKESTER_VERSION "0.1.0")
set(ORKESTER_REPO "calebbuffa/socle")

# ── select the right tarball for this platform ───────────────────────────────

if(VCPKG_TARGET_IS_WINDOWS)
    set(ORKESTER_TRIPLET "x86_64-pc-windows-msvc")
    set(ORKESTER_SHA512 "0")
elseif(VCPKG_TARGET_IS_OSX)
    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
        set(ORKESTER_TRIPLET "aarch64-apple-darwin")
    else()
        set(ORKESTER_TRIPLET "x86_64-apple-darwin")
    endif()
    set(ORKESTER_SHA512 "0")
elseif(VCPKG_TARGET_IS_LINUX)
    set(ORKESTER_TRIPLET "x86_64-unknown-linux-gnu")
    set(ORKESTER_SHA512 "0")
else()
    message(FATAL_ERROR "orkester: unsupported platform")
endif()

set(ORKESTER_URL
    "https://github.com/${ORKESTER_REPO}/releases/download/v${ORKESTER_VERSION}/orkester-${ORKESTER_TRIPLET}.tar.gz")

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

# CMake config
file(INSTALL "${SOURCE_PATH}/lib/cmake/" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    FILES_MATCHING PATTERN "*.cmake")

# License
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Apache-2.0 License")
