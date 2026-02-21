cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
  set(PBA_LLVM_BIN_DIRS
    "/opt/homebrew/opt/llvm/bin"
    "/usr/local/opt/llvm/bin"
  )
  foreach(PBA_LLVM_BIN_DIR IN LISTS PBA_LLVM_BIN_DIRS)
    if(EXISTS "${PBA_LLVM_BIN_DIR}/clang" AND EXISTS "${PBA_LLVM_BIN_DIR}/clang++")
      set(CMAKE_C_COMPILER   "${PBA_LLVM_BIN_DIR}/clang"   CACHE FILEPATH "C compiler" FORCE)
      set(CMAKE_CXX_COMPILER "${PBA_LLVM_BIN_DIR}/clang++" CACHE FILEPATH "C++ compiler" FORCE)
      message(STATUS "Using LLVM toolchain from ${PBA_LLVM_BIN_DIR}")
      break()
    endif()
  endforeach()
endif()

project(pba_viewer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable online FetchContent updates" FORCE)
option(PBA_USE_OPENMP "Enable OpenMP parallel loops where safe" ON)

set(PBA_HOMEBREW_LIBCXX_DIR "")
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  get_filename_component(PBA_CXX_COMPILER_REAL "${CMAKE_CXX_COMPILER}" REALPATH)
  set(PBA_HOMEBREW_LIBCXX_CANDIDATES "")
  if(
    CMAKE_CXX_COMPILER MATCHES "^/opt/homebrew/opt/llvm/bin/clang\\+\\+$"
    OR PBA_CXX_COMPILER_REAL MATCHES "^/opt/homebrew/Cellar/llvm/.+/bin/clang\\+\\+$"
  )
    list(APPEND PBA_HOMEBREW_LIBCXX_CANDIDATES "/opt/homebrew/opt/llvm/lib/c++")
  endif()
  if(
    CMAKE_CXX_COMPILER MATCHES "^/usr/local/opt/llvm/bin/clang\\+\\+$"
    OR PBA_CXX_COMPILER_REAL MATCHES "^/usr/local/Cellar/llvm/.+/bin/clang\\+\\+$"
  )
    list(APPEND PBA_HOMEBREW_LIBCXX_CANDIDATES "/usr/local/opt/llvm/lib/c++")
  endif()
  foreach(PBA_LIBCXX_DIR IN LISTS PBA_HOMEBREW_LIBCXX_CANDIDATES)
    if(EXISTS "${PBA_LIBCXX_DIR}/libc++.1.dylib")
      set(PBA_HOMEBREW_LIBCXX_DIR "${PBA_LIBCXX_DIR}")
      break()
    endif()
  endforeach()
  if(PBA_HOMEBREW_LIBCXX_CANDIDATES AND NOT PBA_HOMEBREW_LIBCXX_DIR)
    message(WARNING "Homebrew clang detected but matching libc++ runtime was not found")
  endif()
  if(PBA_HOMEBREW_LIBCXX_DIR)
    message(STATUS "Using Homebrew libc++ runtime: ${PBA_HOMEBREW_LIBCXX_DIR}")
  endif()
endif()

include(FetchContent)

find_program(FFMPEG_EXECUTABLE NAMES ffmpeg REQUIRED)
message(STATUS "Found ffmpeg: ${FFMPEG_EXECUTABLE}")

FetchContent_Declare(
  ms_gsl
  GIT_REPOSITORY https://github.com/microsoft/GSL.git
  GIT_TAG        v4.0.0
)
FetchContent_MakeAvailable(ms_gsl)

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

FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG        v2.8.21
)
FetchContent_MakeAvailable(tinygltf)

add_library(tinygltf_hdr INTERFACE)
target_include_directories(tinygltf_hdr SYSTEM INTERFACE
  ${tinygltf_SOURCE_DIR}
)
target_link_libraries(tinygltf_hdr INTERFACE
  stb_image_hdr
  nlohmann_json::nlohmann_json
)

add_library(pba_openmp INTERFACE)
set(PBA_OPENMP_ENABLED OFF)
if(PBA_USE_OPENMP)
  find_package(OpenMP QUIET COMPONENTS CXX)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(pba_openmp INTERFACE OpenMP::OpenMP_CXX)
    set(PBA_OPENMP_ENABLED ON)
  elseif(APPLE)
    find_path(PBA_LIBOMP_INCLUDE_DIR
      NAMES omp.h
      HINTS /opt/homebrew/opt/libomp/include /usr/local/opt/libomp/include
    )
    find_library(PBA_LIBOMP_LIBRARY
      NAMES omp libomp
      HINTS /opt/homebrew/opt/libomp/lib /usr/local/opt/libomp/lib
    )
    if(PBA_LIBOMP_INCLUDE_DIR AND PBA_LIBOMP_LIBRARY)
      target_compile_options(pba_openmp INTERFACE -Xpreprocessor -fopenmp)
      target_include_directories(pba_openmp INTERFACE ${PBA_LIBOMP_INCLUDE_DIR})
      target_link_libraries(pba_openmp INTERFACE ${PBA_LIBOMP_LIBRARY})
      set(PBA_OPENMP_ENABLED ON)
      message(STATUS "Using Apple OpenMP fallback: ${PBA_LIBOMP_LIBRARY}")
    endif()
  endif()
endif()
if(PBA_OPENMP_ENABLED)
  message(STATUS "OpenMP parallelism enabled")
else()
  message(STATUS "OpenMP parallelism disabled")
endif()

file(GLOB_RECURSE PBA_SOURCES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/*.cc
)

set(PBA_APP_COMMON_SOURCES
  src/app/headless_main.cpp
)

add_executable(main
  src/app/main.cpp
  ${PBA_APP_COMMON_SOURCES}
  ${PBA_SOURCES}
)

add_executable(headless
  src/app/headless_entry.cpp
  ${PBA_APP_COMMON_SOURCES}
  ${PBA_SOURCES}
)

function(configure_pba_target target_name)
target_compile_definitions(${target_name} PRIVATE
DS_PBA_FFMPEG_EXECUTABLE=\"${FFMPEG_EXECUTABLE}\"
)
if(PBA_OPENMP_ENABLED)
  target_compile_definitions(${target_name} PRIVATE DS_PBA_USE_OPENMP=1)
endif()

target_include_directories(${target_name} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_precompile_headers(${target_name} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pba/core/pch.hpp
)

target_link_libraries(${target_name} PRIVATE
  imgui_lib
  glfw
  glad
  glm::glm

  tinyobjloader_hdr
  tinygltf_hdr
  stb_image_hdr
  nlohmann_json::nlohmann_json

  Microsoft.GSL::GSL
)
if(PBA_HOMEBREW_LIBCXX_DIR)
  target_link_directories(${target_name} PRIVATE ${PBA_HOMEBREW_LIBCXX_DIR})
endif()
if(PBA_OPENMP_ENABLED)
  target_link_libraries(${target_name} PRIVATE pba_openmp)
endif()

target_compile_options(${target_name} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wshadow
  -Wconversion
  -Wfloat-conversion
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
  target_link_libraries(${target_name} PRIVATE "-framework OpenGL")
endif()

add_custom_command(TARGET ${target_name} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          ${CMAKE_CURRENT_BINARY_DIR}/assets
)
endfunction()

configure_pba_target(main)
configure_pba_target(headless)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  add_custom_target(docs_technical
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_technical_docs.sh
    COMMENT "Generating technical API docs with Doxygen"
    VERBATIM
  )
endif()
