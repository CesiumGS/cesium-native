add_library(modp_b64 STATIC modp_b64.cc modp_b64.h modp_b64_data.h)

target_include_directories(
    modp_b64
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
