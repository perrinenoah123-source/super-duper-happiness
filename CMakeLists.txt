cmake_minimum_required(VERSION 3.22)
project(roblox_ssr_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(dobby)

add_library(roblox_ssr SHARED
    main.cpp
    egl_hook.cpp
    ssr_pass_gles.cpp
    overlay_android.cpp
)

target_link_libraries(roblox_ssr
    dobby
    android
    log
    EGL
    GLESv2
    dl
)
