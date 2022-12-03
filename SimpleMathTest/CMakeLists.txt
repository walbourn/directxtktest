# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (simplemathtest
  DESCRIPTION "DirectX Tool Kit Simple Math Test Suite"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

option(BUILD_FOR_ONECORE   "Build for OneCore (Windows 10/Windows 11)" OFF)
option(BUILD_AVX_TEST      "Build for /arch:AVX" OFF)
option(BUILD_AVX2_TEST     "Build for /arch:AVX2" OFF)
option(BUILD_DISABLE_SVML  "Disable use of SVML (VS 2019)" OFF)
option(BUILD_NO_INTRINSICS "Disable use of compiler intrinsics" OFF)

if (MINGW OR (NOT WIN32))
   set(BUILD_FOR_ONECORE OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if (DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

set(DXMATH_DEFS "$<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>")

if (BUILD_NO_INTRINSICS)
    message("Building with compiler intrinsics disabled (_XM_NO_INTRINSICS_)")
    list(APPEND DXMATH_DEFS _XM_NO_INTRINSICS_)
elseif (BUILD_DISABLE_SVML)
    list(APPEND DXMATH_DEFS _XM_DISABLE_INTEL_SVML_)
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    set(ARCH_AVX   $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx>)
    set(ARCH_AVX2  $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2 -mfma -mf16c>)
endif()

if(MSVC AND BUILD_FOR_ONECORE)
    message("INFO: Building for OneCore (Windows 10)")

    # Locate Visual Studio (needed for VC Runtime DLLs)
    if (NOT DEFINED VCInstallDir AND DEFINED ENV{VCINSTALLDIR})
        set(VCInstallDir $ENV{VCINSTALLDIR})
    endif()

    if (NOT DEFINED VCInstallDir)
        set(VS_EDITIONS "Community" "Professional" "Enterprise" "Preview" "BuildTools")
        foreach(vsedition IN LISTS VS_EDITIONS)
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()

        if (NOT EXISTS ${VCInstallDir})
            foreach(vsedition IN LISTS VS_EDITIONS)
                set(VCInstallDir "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/${vsedition}/VC")
                if(EXISTS ${VCInstallDir})
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(EXISTS ${VCInstallDir})
        message("VCInstallDir = ${VCInstallDir}")
    else()
        message(FATAL_ERROR "ERROR: Failed to locate Visual Studio 2019 or 2022 install")
    endif()

    # Find VC toolset/runtime versions
    file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" VCToolsVersion)
    message("VCToolsVersion = ${VCToolsVersion}")

    # Don't link with kernel32.lib, etc.
    set(CMAKE_CXX_STANDARD_LIBRARIES "")
    set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")

    # Need to link with "onecore" versions of Visual C++ libraries
    set(VC_OneCore_LibPath "${VCInstallDir}/Tools/MSVC/${VCToolsVersion}/lib/onecore/${DIRECTX_ARCH}")
    if(NOT EXISTS ${VC_OneCore_LibPath}/msvcrt.lib)
        message(FATAL_ERROR "ERROR: Cannot locate msvcrt.lib for the Visual C++ toolset (${VCToolsVersion})")
    endif()

    list(APPEND DXMATH_DEFS "_WIN32_WINNT=0x0A00")
endif()

set(TEST_SOURCES
    SimpleMathTest.cpp
    SimpleMathTestD3D12.cpp
    )

set(TEST_INCLUDE_DIR ./)

if (WIN32)
    list(APPEND DXMATH_DEFS "TEST_D3D11")
    set(TEST_SOURCES ${TEST_SOURCES} SimpleMathTestD3D11.cpp)
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../Inc/SimpleMath.h")
        message("INFO: Using SimpleMath from DirectX Tool Kit.")
        set(TEST_SOURCES ${TEST_SOURCES} ../../Inc/SimpleMath.h ../../Inc/SimpleMath.inl ../../Src/SimpleMath.cpp)
        set(TEST_INCLUDE_DIR ${TEST_INCLUDE_DIR} ../../Inc)
   else()
        message("INFO: Using SimpleMath standalone from GitHub.")

        include(FetchContent)
        FetchContent_Declare(
            simplemathhpp
            URL https://raw.githubusercontent.com/microsoft/DirectXTK/main/Inc/SimpleMath.h
            URL_HASH SHA256=e67b7c05a444b0f571e42e3fdc509e9d04a17f57c67e5f5751f7519ec68add0c
            DOWNLOAD_NO_EXTRACT ON
        )
        FetchContent_Declare(
            simplemathinl
            URL https://raw.githubusercontent.com/microsoft/DirectXTK/main/Inc/SimpleMath.inl
            URL_HASH SHA256=cfcba37fc5e03f532f586ada84c7a0baf566c865b85e75d82b885114982e4765
            DOWNLOAD_NO_EXTRACT ON
        )
        FetchContent_Declare(
            simplemathcpp
            URL https://raw.githubusercontent.com/microsoft/DirectXTK/main/Src/SimpleMath.cpp
            URL_HASH SHA256=c148cd0c84ff8c972b02e8c92bb5e86a03d74e7817604b66efe8cdf7952d6edd
            DOWNLOAD_NO_EXTRACT ON
        )
        FetchContent_MakeAvailable(simplemathhpp simplemathinl simplemathcpp)

        set(TEST_SOURCES ${TEST_SOURCES}
            ${simplemathhpp_SOURCE_DIR}/SimpleMath.h
            ${simplemathinl_SOURCE_DIR}/SimpleMath.inl
            ${simplemathcpp_SOURCE_DIR}/SimpleMath.cpp)

        configure_file(SimpleMathStandalone.in pch.h COPYONLY)
        set(TEST_INCLUDE_DIR ${TEST_INCLUDE_DIR} ${simplemathhpp_SOURCE_DIR} ${simplemathinl_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
   endif()
endif()

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${TEST_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DXMATH_DEFS})

if (MINGW OR (NOT WIN32) OR VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectX-Headers and DirectXMath.")
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectXMath)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /GR /fp:fast)
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token" "-Wno-reserved-id-macro"
                    "-Wno-missing-prototypes" "-Wno-missing-variable-declarations"
                    "-Wno-double-promotion" "-Wno-unused-variable" "-Wno-float-equal")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
elseif( MINGW )
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
elseif ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus)

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4571" "/wd4668" "/wd4710" "/wd4820" "/wd5039" "/wd5045" "/wd5262" "/wd5264")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

    if (BUILD_AVX2_TEST)
        message("INFO: Building for AVX2")
        target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX2})
    elseif (BUILD_AVX_TEST)
        message("INFO: Building for AVX")
        target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX})
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
    endif()
endif()

if (MSVC AND BUILD_FOR_ONECORE)
    target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath})
    target_link_libraries(${PROJECT_NAME} onecore_apiset.lib)
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:CONSOLE,10.0 /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:onecore.lib)
endif()
