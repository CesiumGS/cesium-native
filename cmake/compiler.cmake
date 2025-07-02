# if (MSVC)
#     add_compile_options(/W4 /WX /wd4201 /bigobj)
# else()
#     add_compile_options(-Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
# endif()

if(CESIUM_TARGET_WASM)
  add_compile_options(-sMEMORY64=1 -pthread -msimd128 -mnontrapping-fptoint -fwasm-exceptions -sSUPPORT_LONGJMP=wasm)
  add_link_options(-sMEMORY64=1 -pthread -sALLOW_MEMORY_GROWTH=1 -sPTHREAD_POOL_SIZE=4 -sMAXIMUM_MEMORY=8589934592 -sMIN_NODE_VERSION=200000 -sINITIAL_MEMORY=268435456 -sSTACK_SIZE=1048576 -fwasm-exceptions -mbulk-memory -mnontrapping-fptoint -msse4.2 -sMALLOC=mimalloc -sWASM_BIGINT -sSUPPORT_LONGJMP=wasm -sFORCE_FILESYSTEM -sPROXY_TO_PTHREAD)

  add_link_options($<$<CONFIG:Debug>:-gsource-map>)
endif()