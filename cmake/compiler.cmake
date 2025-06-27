# if (MSVC)
#     add_compile_options(/W4 /WX /wd4201 /bigobj)
# else()
#     add_compile_options(-Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
# endif()

if(CESIUM_TARGET_WASM32)
  # MEMORY64 = 2 means clang will compile for 64bits but 32bit wasm will be produced (using i64 for pointers).
  # This option provides the most compatibility until full Memory64 support is in most browsers.
  add_compile_options(-sMEMORY64=2)
  add_link_options(-sMEMORY64=2 -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=8589934592)
endif()