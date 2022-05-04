# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)

project (headertest
  DESCRIPTION "DirectX Tool Kit for DirectX 11 Header Inclusion Test"
  HOMEPAGE_URL "http://go.microsoft.com/fwlink/?LinkId=248929"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if(MSVC)
    # Use max Warning Level 
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

add_executable(${PROJECT_NAME}
    main.cpp
    audiotest.cpp
    bufferhelperstest.cpp
    commonstatestest.cpp
    ddstextureloadertest.cpp
    directxhelperstest.cpp
    effectstest.cpp
    gamepadtest.cpp
    geometricprimitivetest.cpp
    graphicsmemorytest.cpp
    keyboardtest.cpp
    modeltest.cpp
    mousetest.cpp
    postprocesstest.cpp
    primitivebatchtest.cpp
    screengrabtest.cpp
    simplemathtest.cpp
    spritebatchtest.cpp
    spritefonttest.cpp
    vertextypestest.cpp
    wictextureloadertest.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

if(MSVC)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
    endif()
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /sdl /permissive- /Zc:__cplusplus)

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4571" "/wd4668" "/wd4710" "/wd4820" "/wd5031" "/wd5032" "/wd5039" "/wd5045" "/wd5219")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    # Windows 8 is used here to use XAudio 2.8 rather than require the legacy DirectX SDK or NuGet (https://aka.ms/XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0602)
endif()
