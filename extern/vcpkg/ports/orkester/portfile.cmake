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
    set(ORKESTER_SHA512 "d9a9eda7a5b1cdabbdf5c84cbd6761733d07607a49101ec6100bbee85a80eceb8da3c3f312197d8ed52e3cc3524ea12e1d9157ea66afd16edfbc4cb456f754b4")
elseif(VCPKG_TARGET_IS_OSX)
    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
        set(ORKESTER_TRIPLET "aarch64-apple-darwin")
        set(ORKESTER_SHA512 "8d2f72614e5049d392f23ebe77ad487798bb4178d4e9e5d5b8ae58675581f62095d6a327c31f6ce8852472b4b2957a47cccbd696340241deb90aa0740046bdaa")
    else()
        set(ORKESTER_TRIPLET "x86_64-apple-darwin")
        set(ORKESTER_SHA512 "e8cc0dc8c95c3df786d4cb3d19e9ac6c358cd8764be277b82981f6c5de1630c0c72aa982d9ec2325404ab81fd6389b6731e894d350a99e5f1a1dd88c44963736")
    endif()
elseif(VCPKG_TARGET_IS_LINUX)
    set(ORKESTER_TRIPLET "x86_64-unknown-linux-gnu")
    set(ORKESTER_SHA512 "d801a0a3afcb7c3c93f7890bfacc5434973232406e1056f6795cd66cd9f7946034bd5a95a21c431f5a66a95edbeb2203273fe951edf0e15348cbdc47dddc17c7")
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

# CMake config
file(INSTALL "${SOURCE_PATH}/lib/cmake/" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    FILES_MATCHING PATTERN "*.cmake")

# License
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Apache-2.0 License")
