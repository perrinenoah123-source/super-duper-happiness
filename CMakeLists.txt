cmake_minimum_required(VERSION 3.22)
project(roblox_ssr_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(roblox_ssr SHARED
    src/main.cpp
    src/egl_hook.cpp
    src/ssr_pass_gles.cpp
    src/overlay_android.cpp
)

target_include_directories(roblox_ssr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(roblox_ssr PRIVATE ANDROID _FORTIFY_SOURCE=2)
target_compile_options(roblox_ssr PRIVATE -Wall -Wextra -Wpedantic)

find_library(ANDROID_LOG_LIB log REQUIRED)
find_library(ANDROID_LIB android REQUIRED)
find_library(EGL_LIB EGL REQUIRED)
find_library(GLES_LIB GLESv3 REQUIRED)

target_link_libraries(roblox_ssr PRIVATE
    ${ANDROID_LOG_LIB}
    ${ANDROID_LIB}
    ${EGL_LIB}
    ${GLES_LIB}
    dl
)

set_target_properties(roblox_ssr PROPERTIES
    OUTPUT_NAME "roblox_ssr"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
