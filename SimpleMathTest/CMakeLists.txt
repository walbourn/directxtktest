# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.13)

project (simplemathtest
  DESCRIPTION "DirectX Tool Kit Simple Math Test Suite"
  HOMEPAGE_URL "http://go.microsoft.com/fwlink/?LinkId=248929"
  LANGUAGES CXX)

option(BUILD_FOR_ONECORE   "Build for OneCore (Windows 10/Windows 11)" OFF)
option(BUILD_AVX_TEST      "Build for /arch:AVX" OFF)
option(BUILD_AVX2_TEST     "Build for /arch:AVX2" OFF)
option(BUILD_DISABLE_SVML  "Disable use of SVML (VS 2019)" OFF)
option(BUILD_NO_INTRINSICS "Disable use of compiler intrinsics" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if (NOT DXMATH_ARCHITECTURE)
    set(DXMATH_ARCHITECTURE "x64")
endif()

if (BUILD_NO_INTRINSICS)
    message("Building with compiler intrinsics disabled (_XM_NO_INTRINSICS_)")
    set(DXMATH_DEFS _XM_NO_INTRINSICS_)
elseif (BUILD_DISABLE_SVML)
    set(DXMATH_DEFS _XM_DISABLE_INTEL_SVML_)
endif()

if (NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    set(ARCH_AVX   $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx>)
    set(ARCH_AVX2  $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2 -mfma -mf16c>)
endif()

if(MSVC)
    # Use max Warning Level 
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Not using typeid or dynamic_cast, so disable RTTI to save binary size
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # General settings for all MSVC-like targets
    if (BUILD_FOR_ONECORE)
        message("INFO: Building for OneCore (Windows 10)")
        # Locate Visual Studio (needed for VC Runtime DLLs)
        set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Community/VC")
        if(NOT EXISTS ${VCInstallDir})
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Professional/VC")
        endif()
        if(NOT EXISTS ${VCInstallDir})
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Enterprise/VC")
        endif()
        if(NOT EXISTS ${VCInstallDir})
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Preview/VC")
        endif()
        if(EXISTS ${VCInstallDir})
            message("VCInstallDir = ${VCInstallDir}")
        else()
            message(FATAL_ERROR "ERROR: Failed to locate Visual Studio 2019 install")
        endif()

        # Find VC toolset/runtime versions
        file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" VCToolsVersion)
        message("VCToolsVersion = ${VCToolsVersion}")

        # Don't link with kernel32.lib, etc.
        set(CMAKE_CXX_STANDARD_LIBRARIES "")
        set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")

        # Need to link with "onecore" versions of Visual C++ libraries
        set(VC_OneCore_LibPath "${VCInstallDir}/Tools/MSVC/${VCToolsVersion}/lib/onecore/${DXMATH_ARCHITECTURE}")
        if(NOT EXISTS ${VC_OneCore_LibPath}/msvcrt.lib)
            message(FATAL_ERROR "ERROR: Cannot locate msvcrt.lib for the Visual C++ toolset (${VCToolsVersion})")
        endif()

        list(APPEND DXMATH_DEFS "_WIN32_WINNT=0x0A00")
    endif()
endif()

add_executable(${PROJECT_NAME}
    SimpleMathTest.cpp
    SimpleMathTestD3D11.cpp
    SimpleMathTestD3D12.cpp
    ../../Src/SimpleMath.cpp)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)
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

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DXMATH_DEFS})

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

    if (NOT MINGW)
        if (BUILD_AVX2_TEST)
           target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX2})
        elseif (BUILD_AVX_TEST)
           target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX})
        else()
           target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
        endif()
    endif()
endif()

if (MSVC AND BUILD_FOR_ONECORE)
    target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath})
    target_link_libraries(${PROJECT_NAME} onecore_apiset.lib)
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:CONSOLE,10.0 /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:onecore.lib)
endif()
