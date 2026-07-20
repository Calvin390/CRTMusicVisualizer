cmake_minimum_required(VERSION 4.2)

project(CRT_Visualizer LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(cJSON STATIC
        ExternalLibs/cJSON-1.7.19/cJSON.c
)

target_include_directories(cJSON
        PUBLIC
        "C:/Users/cbumgardner/CLionProjects/CRT_Visualizer/ExternalLibs/cJSON-1.7.19"
)

set(SDL3_BASE
        "C:/Users/cbumgardner/CLionProjects/CRT_Visualizer/ExternalLibs/SDL3-3.4.12"
)

set(SDL3_IMAGE_BASE
        "C:/Users/cbumgardner/CLionProjects/CRT_Visualizer/ExternalLibs/SDL3_image-3.4.4"
)

set(SDL3_TTF_BASE
        "C:/Users/cbumgardner/CLionProjects/CRT_Visualizer/ExternalLibs/SDL3_ttf-3.2.2"
)

# Exact folders containing the package configuration files.
set(SDL3_DIR
        "${SDL3_BASE}/cmake"
)

set(SDL3_image_DIR
        "${SDL3_IMAGE_BASE}/cmake"
)

set(SDL3_ttf_DIR
        "${SDL3_TTF_BASE}/cmake"
)

# Architecture-specific runtime folders.
set(SDL3_RUNTIME
        "${SDL3_BASE}/x86_64-w64-mingw32"
)

set(SDL3_IMAGE_RUNTIME
        "${SDL3_IMAGE_BASE}/x86_64-w64-mingw32"
)

set(SDL3_TTF_RUNTIME
        "${SDL3_TTF_BASE}/x86_64-w64-mingw32"
)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

add_executable(CRT_Visualizer
        main.c
)

target_link_libraries(CRT_Visualizer
        PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        cJSON
)

if(MINGW)
    target_compile_options(CRT_Visualizer
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_custom_command(
        TARGET CRT_Visualizer
        POST_BUILD

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${SDL3_RUNTIME}/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:CRT_Visualizer>"

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${SDL3_IMAGE_RUNTIME}/bin/SDL3_image.dll"
        "$<TARGET_FILE_DIR:CRT_Visualizer>"

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${SDL3_TTF_RUNTIME}/bin/SDL3_ttf.dll"
        "$<TARGET_FILE_DIR:CRT_Visualizer>"

        COMMENT "Copying SDL runtime DLLs"
        VERBATIM
)