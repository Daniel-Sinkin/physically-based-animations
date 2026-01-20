cmake_minimum_required(VERSION 3.20)
project(pba_viewer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_library(glad STATIC
  extern/glad/src/glad.c
)
target_include_directories(glad PUBLIC
  extern/glad/include
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw glad)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)


FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG        release
)
FetchContent_MakeAvailable(tinyobjloader)

add_library(tinyobjloader_hdr INTERFACE)
target_include_directories(tinyobjloader_hdr SYSTEM INTERFACE
  ${tinyobjloader_SOURCE_DIR}
)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

add_library(stb_image_hdr INTERFACE)
target_include_directories(stb_image_hdr SYSTEM INTERFACE
  ${stb_SOURCE_DIR}
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)


file(GLOB_RECURSE PBA_SOURCES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.cc
)

add_executable(app
  src/app/main.cpp
  ${PBA_SOURCES}
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_precompile_headers(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/pch.hpp
)

target_link_libraries(app PRIVATE
  imgui_lib
  glfw
  glad
  glm::glm

  tinyobjloader_hdr
  stb_image_hdr
  nlohmann_json::nlohmann_json
)

target_compile_options(app PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wcast-qual
  -Wcast-align
  -Wformat=2
  -Wundef
  -Wnull-dereference
  -Wnon-virtual-dtor
  -Woverloaded-virtual
  -Wold-style-cast
  -Wzero-as-null-pointer-constant
  -Wdouble-promotion
  -Wimplicit-fallthrough
  -Wreturn-type
)

if(APPLE)
  target_link_libraries(app PRIVATE "-framework OpenGL")
endif()

add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          ${CMAKE_CURRENT_BINARY_DIR}/assets
)
