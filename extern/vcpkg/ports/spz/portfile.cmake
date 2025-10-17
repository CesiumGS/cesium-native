vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO nianticlabs/spz
    REF "8767f22a39c32f8190b2e0f9ba256516c74e7b73"
    SHA512 fb0154b8154e252d08b9751df3b66e75baf21cd4c9cec9c8de520f7cee62e86ab1d27f4d5a64f44e8bdac7b72fe465860181aa5b726c252742c6611ed91d9437
    HEAD_REF main
    PATCHES
      loadSpz.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_SHARED_LIBS=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
