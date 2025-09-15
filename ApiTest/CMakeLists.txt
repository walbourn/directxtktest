# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.21)

project (apitest
  DESCRIPTION "DirectX Tool Kit for DX11 API Test"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
  message(FATAL_ERROR "DirectX Tool Kit Test Suite should be built by the main CMakeLists")
endif()

set(SOURCES
  ApiTest.cpp
  bufferhelpers.cpp
  commonstates.cpp
  directxhelpers.cpp
  effects.cpp
  graphicsmemory.cpp
  loaderhelpers.cpp
  model.cpp
  postprocess.cpp
  primitivebatch.cpp
  primitives.cpp
  shared.cpp
  sprites.cpp
  vertextypes.cpp)

if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_REDIST)
  list(APPEND SOURCES audio.cpp)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_REDIST)
  target_compile_definitions(${PROJECT_NAME} PRIVATE TEST_AUDIO)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ../../Src)

target_link_libraries(${PROJECT_NAME} PRIVATE DirectXTK d3d11.lib)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc /GR)
endif()

if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-float-equal" "-Wno-global-constructors" "-Wno-language-extension-token" "-Wno-missing-prototypes" "-Wno-missing-variable-declarations" "-Wno-reserved-id-macro" "-Wno-unused-macros" "-Wno-switch-enum")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsEXE "-Wno-unsafe-buffer-usage")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-attributes" "-Walloc-size-larger-than=4GB")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(WarningsEXE /wd4061 /wd4365 /wd4668 /wd4710 /wd4820 /wd5031 /wd5032 /wd5039 /wd5045)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE /wd5262 /wd5264)
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=${WINVER})
endif()
