set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH EM_CONFIG)

if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
   find_path(EMSCRIPTEN_ROOT "emcc")
else()
   set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
endif()

if(NOT EMSCRIPTEN_ROOT)
   if(NOT DEFINED ENV{EMSDK})
      message(FATAL_ERROR "The emcc compiler not found in PATH")
   endif()
   set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
endif()

if(NOT EXISTS "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
   message(FATAL_ERROR "Emscripten.cmake toolchain file not found")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")

# These lines are the key difference between this triplet and the wasm32-emscripten that comes with vcpkg.
# They set specific compiler and linker flags needed to use cesium-native on the web, particularly as part of Unity.
set(_configureFlags "-pthread -msimd128 -mnontrapping-fptoint -fwasm-exceptions -sSUPPORT_LONGJMP=wasm -DSIZEOF_SIZE_T=4")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_C_FLAGS=${_configureFlags} -DCMAKE_CXX_FLAGS=${_configureFlags} -DCMAKE_EXE_LINKER_FLAGS=${_configureFlags})
