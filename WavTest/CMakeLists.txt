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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

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
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
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

if (BUILD_XAUDIO_WIN7 AND xaudio2redist_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()

  if((MSVC_VERSION GREATER_EQUAL 1924)
     AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
    target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
  endif()

  if((MSVC_VERSION GREATER_EQUAL 1928)
    AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
    AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
      target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-float-equal" "-Wno-global-constructors" "-Wno-language-extension-token" "-Wno-missing-prototypes" "-Wno-missing-variable-declarations" "-Wno-reserved-id-macro" "-Wno-unused-macros" "-Wno-switch-enum")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-unsafe-buffer-usage")
    endif()
elseif(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition -municode)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus /Zc:inline)

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4668" "/wd4710" "/wd4820" "/wd5031" "/wd5032" "/wd5039" "/wd5045" )
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
        list(APPEND WarningsEXE "/wd5262" "/wd5264")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT ((${DIRECTX_ARCH} MATCHES "^arm"))))
        target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()

    if (BUILD_FUZZING AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=fuzzer /fsanitize=address /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
        target_link_libraries(${PROJECT_NAME} PRIVATE sancov.lib)
        target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4291)
    endif()
endif()

if(WIN32)
    if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10)
        set(WINVER 0x0A00)
    elseif((${DIRECTX_ARCH} MATCHES "^arm64") OR BUILD_GAMEINPUT OR BUILD_WGI)
        set(WINVER 0x0A00)
    elseif(BUILD_XAUDIO_WIN8)
        set(WINVER 0x0602)
    elseif(${DIRECTX_ARCH} MATCHES "^arm")
        set(WINVER 0x0602)
    else()
        set(WINVER 0x0601)
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN7_PLATFORM_UPDATE)
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})
endif()
