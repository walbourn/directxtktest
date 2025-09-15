# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.21)

project (simplemathtest
  DESCRIPTION "DirectX Tool Kit Simple Math Test Suite"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

option(BUILD_FOR_ONECORE   "Build for OneCore (Windows 10/Windows 11)" OFF)
option(BUILD_AVX_TEST      "Build for /arch:AVX" OFF)
option(BUILD_AVX2_TEST     "Build for /arch:AVX2" OFF)
option(BUILD_DISABLE_SVML  "Disable use of SVML (VS 2019)" OFF)
option(BUILD_NO_INTRINSICS "Disable use of compiler intrinsics" OFF)

if(MINGW OR (NOT WIN32))
   set(BUILD_FOR_ONECORE OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64EC$")
    set(DIRECTX_ARCH arm64ec)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64EC$")
    set(DIRECTX_ARCH arm64ec)
endif()

if(BUILD_NO_INTRINSICS)
    message("Building with compiler intrinsics disabled (_XM_NO_INTRINSICS_)")
    set(DXMATH_DEFS _XM_NO_INTRINSICS_)
elseif(BUILD_DISABLE_SVML)
    set(DXMATH_DEFS _XM_DISABLE_INTEL_SVML_)
endif()

if(MSVC AND BUILD_FOR_ONECORE)
    message("INFO: Building for OneCore (Windows 10)")

    # Locate Visual Studio (needed for VC Runtime DLLs)
    if(NOT DEFINED VCInstallDir AND DEFINED ENV{VCINSTALLDIR})
        set(VCInstallDir $ENV{VCINSTALLDIR})
    endif()

    if(NOT DEFINED VCInstallDir)
        set(VS_EDITIONS "Community" "Professional" "Enterprise" "Preview" "BuildTools")
        foreach(vsedition IN LISTS VS_EDITIONS)
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()

        if(NOT EXISTS ${VCInstallDir})
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

set(TEST_INCLUDE_DIR ./)

set(TEST_SOURCES SimpleMathTest.cpp)

if(WIN32)
    set(TEST_SOURCES ${TEST_SOURCES} SimpleMathTestD3D11.cpp)
    list(APPEND DXMATH_DEFS "TEST_D3D11")
endif()

if(NOT BUILD_SHARED_LIBS)
    set(TEST_SOURCES ${TEST_SOURCES} SimpleMathTestD3D12.cpp)
    list(APPEND DXMATH_DEFS "TEST_D3D12")
endif()

if(PROJECT_IS_TOP_LEVEL)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../Inc/SimpleMath.h")
        if(WIN32)
            set(TEST_SOURCES ${TEST_SOURCES} ../../Inc/SimpleMath.h ../../Inc/SimpleMath.inl ../../Src/SimpleMath.cpp)
            set(TEST_INCLUDE_DIR ${TEST_INCLUDE_DIR} ../../Inc)
        else()
          configure_file(SimpleMathStandalone.in pch.h COPYONLY)
          configure_file(../../Inc/SimpleMath.h SimpleMath.h COPYONLY)
          configure_file(../../Inc/SimpleMath.inl SimpleMath.inl COPYONLY)
          configure_file(../../Src/SimpleMath.cpp SimpleMath.cpp COPYONLY)
          set(TEST_SOURCES ${TEST_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/SimpleMath.h ${CMAKE_CURRENT_BINARY_DIR}/SimpleMath.inl ${CMAKE_CURRENT_BINARY_DIR}/SimpleMath.cpp)
          set(TEST_INCLUDE_DIR  ${TEST_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
        endif()
   else()
        message(FATAL_ERROR "ERROR: SimpleMath must be 'above' this project")
   endif()
endif()

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${TEST_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DXMATH_DEFS})

if(MINGW OR (NOT WIN32))
    find_package(directxmath CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
else()
    find_package(directxmath CONFIG QUIET)
    find_package(directx-headers CONFIG QUIET)
endif()

if(directxmath_FOUND)
    message(STATUS "Using DirectXMath package")
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)
endif()

if(directx-headers_FOUND)
    message(STATUS "Using DirectX-Headers package")
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /EHsc /GR "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
      target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
        AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
        AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
          target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
          target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()
else()
    add_compile_definitions(PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC,Intel>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-msse2>)
    endif()

    set(ARCH_AVX   $<$<CXX_COMPILER_ID:MSVC,Intel>:/arch:AVX>  $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-mavx>)
    set(ARCH_AVX2  $<$<CXX_COMPILER_ID:MSVC,Intel>:/arch:AVX2> $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-mavx2 -mfma -mf16c>)
endif()

if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    if(MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0))
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token" "-Wno-reserved-id-macro"
        "-Wno-missing-prototypes" "-Wno-missing-variable-declarations"
        "-Wno-double-promotion" "-Wno-unused-variable" "-Wno-float-equal")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsEXE "-Wno-unsafe-buffer-usage")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wno-reserved-id-macro" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-gnu-anonymous-struct" "-Wno-ignored-attributes" "-Wno-global-constructors"
        "-Wno-nested-anon-types")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /Zc:inline /fp:fast)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus /Zc:inline /fp:fast)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    set(WarningsEXE /wd4061 /wd4365 /wd4514 /wd4571 /wd4668 /wd4710 /wd4820 /wd5039 /wd5045)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      list(APPEND WarningsEXE /wd5105)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT ((${DIRECTX_ARCH} MATCHES "^arm"))))
      target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE /wd5262 /wd5264)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.50)
      list(APPEND WarningsEXE /wd4865)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

    if(BUILD_AVX2_TEST)
      message("INFO: Building for AVX2")
      target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX2})
    elseif(BUILD_AVX_TEST)
      message("INFO: Building for AVX")
      target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_AVX})
    else()
      target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
    endif()
endif()

if(MSVC AND BUILD_FOR_ONECORE)
    target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath})
    target_link_libraries(${PROJECT_NAME} onecore_apiset.lib)
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:CONSOLE,10.0 /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:onecore.lib)
endif()
