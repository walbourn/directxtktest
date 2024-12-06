# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (wavtest
  DESCRIPTION "DirectX Tool Kit for DX11 Audio File I/O Test"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "DirectX Tool Kit Test Suite should be built by the main CMakeLists")
endif()

add_executable(${PROJECT_NAME}
  WavTest.cpp
  wav.cpp
  xwb.cpp
  ../../Audio/WAVFileReader.h
  ../../Audio/WaveBankReader.h
  )

target_include_directories(${PROJECT_NAME} PRIVATE ../../Audio ../../Src)

target_link_libraries(${PROJECT_NAME} PRIVATE DirectXTK bcrypt.lib)

if(xaudio2redist_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

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
    set(WarningsEXE "/wd4061" "/wd4365" "/wd4668" "/wd4710" "/wd4820" "/wd5031" "/wd5032" "/wd5039" "/wd5045" )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE "/wd5262" "/wd5264")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=${WINVER})
endif()
