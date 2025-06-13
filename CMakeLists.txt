cmake_minimum_required(VERSION 3.24)
project(mpp)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})

make_directory(${BINARY_DIR})

add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_ttf)

set(GLM_BUILD_LIBRARY OFF)
add_subdirectory(lib/glm)

add_executable(mpp WIN32
    lib/fast_obj/fast_obj.c
    lib/sqlite3/sqlite3.c
    lib/stb/stb.c

    src/main.cpp
    src/render_util.cpp
)

set_target_properties(mpp PROPERTIES CXX_STANDARD 20)

target_include_directories(mpp PRIVATE lib/fast_obj)
target_include_directories(mpp PRIVATE lib/jsmn)
target_include_directories(mpp PRIVATE lib/sqlite3)
target_include_directories(mpp PRIVATE lib/stb)

target_link_libraries(mpp PRIVATE SDL3::SDL3)
target_link_libraries(mpp PRIVATE SDL3_ttf::SDL3_ttf)

target_link_libraries(mpp PRIVATE glm)

if(MSVC)
    set(SHADERCROSS lib/SDL_shadercross/msvc/shadercross.exe)
else()
    message("Unsupported platform. Skipping shader building")
endif()

function(add_shader FILE)
    set(DEPENDS ${ARGN})
    set(GLSL ${CMAKE_SOURCE_DIR}/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/bin/${FILE}.spv)
    set(DXIL ${CMAKE_SOURCE_DIR}/bin/${FILE}.dxil)
    set(MSL ${CMAKE_SOURCE_DIR}/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/bin/${FILE}.json)

    function(_add_shader PROGRAM SOURCE OUTPUT)
        if(MSVC)
            add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${PROGRAM} ${SOURCE} -o ${OUTPUT}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS ${SOURCE} ${DEPENDS}
                COMMENT ${OUTPUT}
            )
        endif()

        get_filename_component(NAME ${OUTPUT} NAME)
        set(BINARY ${BINARY_DIR}/${NAME})
        add_custom_command(
            OUTPUT ${BINARY}
            COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${BINARY}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${OUTPUT}
            COMMENT ${BINARY}
        )

        string(REPLACE . _ NAME ${NAME})
        add_custom_target(${NAME} DEPENDS ${BINARY})
        add_dependencies(particles ${NAME})
    endfunction()

    _add_shader(glslc ${GLSL} ${SPV})
    _add_shader(${SHADERCROSS} ${SPV} ${DXIL})
    _add_shader(${SHADERCROSS} ${SPV} ${MSL})
    _add_shader(${SHADERCROSS} ${SPV} ${JSON})
endfunction()