cmake_minimum_required(VERSION 3.24)
project(minicraft++)

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
add_subdirectory(lib/fast_obj)
add_subdirectory(lib/glm)
add_subdirectory(lib/json)
add_subdirectory(lib/stb)
add_executable(minicraft++ WIN32
    src/main.cpp
    src/window.cpp
)
set_target_properties(minicraft++ PROPERTIES CXX_STANDARD 23)
target_link_libraries(minicraft++ PRIVATE SDL3::SDL3)
target_link_libraries(minicraft++ PRIVATE SDL3_ttf::SDL3_ttf)
target_link_libraries(minicraft++ PRIVATE fast_obj)
target_link_libraries(minicraft++ PRIVATE glm-header-only)
target_link_libraries(minicraft++ PRIVATE nlohmann_json)
target_link_libraries(minicraft++ PRIVATE stb)

function(add_shader FILE)
    set(DEPENDS ${ARGN})
    set(GLSL ${CMAKE_SOURCE_DIR}/res/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.spv)
    set(MSL ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.json)

    # TODO: Shaders are only compiled on MSVC
    function(compile PROGRAM SOURCE OUTPUT)
        add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${PROGRAM} ${SOURCE} -o ${OUTPUT}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${SOURCE} ${DEPENDS}
            COMMENT ${OUTPUT}
        )
        get_filename_component(NAME ${OUTPUT} NAME)
        string(REPLACE . _ NAME ${NAME})
        set(NAME compile_${NAME})
        add_custom_target(${NAME} DEPENDS ${OUTPUT})
        add_dependencies(minicraft++ ${NAME})
    endfunction()
    if (MSVC)
        set(SHADERCROSS lib/SDL_shadercross/msvc/shadercross.exe)
        compile(glslc ${GLSL} ${SPV})
        compile(${SHADERCROSS} ${SPV} ${MSL})
        compile(${SHADERCROSS} ${SPV} ${JSON})
    endif()

    # Only package shaders for the target platform
    function(package OUTPUT)
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
        set(NAME package_${NAME})
        add_custom_target(${NAME} DEPENDS ${BINARY})
        add_dependencies(minicraft++ ${NAME})
    endfunction()
    if(APPLE)
        package(${MSL})
    else()
        package(${SPV})
    endif()
    package(${JSON})
endfunction()

configure_file(LICENSE.txt ${BINARY_DIR} COPYONLY)
configure_file(README.md ${BINARY_DIR} COPYONLY)