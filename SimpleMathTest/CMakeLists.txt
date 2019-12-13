# DirectX Tool Kit for DirectX 11 Simple Math Test
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# http://go.microsoft.com/fwlink/?LinkID=615561

cmake_minimum_required (VERSION 3.11)

project (simplemathtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

if(MSVC)
    # Use max Warning Level 
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

add_executable(${PROJECT_NAME}
    SimpleMathTest.cpp
    ../../Src/SimpleMath.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)

    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
    endif()
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token" "-Wno-reserved-id-macro"
                    "-Wno-missing-prototypes" "-Wno-double-promotion" "-Wno-unused-variable" "-Wno-float-equal")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus)

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4571" "/wd4668" "/wd4710" "/wd4820" "/wd5039" "/wd5045")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)
endif()
