# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (fuzzloaders
  DESCRIPTION "DirectX Tool Kit File Fuzzer"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "DirectX Tool Kit Fuzz Tester should be built by the main CMakeLists")
endif()

add_executable(${PROJECT_NAME} fuzzloaders.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ../../Audio)

if(BUILD_FUZZING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FUZZING_BUILD_MODE)
endif()

if(MINGW)
    find_package(directxmath CONFIG REQUIRED)
else()
    find_package(directxmath CONFIG QUIET)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE DirectXTK bcrypt.lib d3d11.lib)

if(directxmath_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)
endif()

if(BUILD_XAUDIO_WIN7 AND xaudio2redist_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /EHsc /GR /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
      target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
       AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND NOT ENABLE_OPENEXR_SUPPORT
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
        target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
        target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

if(DEFINED ARCH_SSE2)
    target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
endif()

if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    if(MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0))
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    set(WarningsEXE "-Wpedantic" "-Wextra" "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-float-equal" "-Wno-global-constructors" "-Wno-language-extension-token" "-Wno-missing-prototypes" "-Wno-missing-variable-declarations" "-Wno-reserved-id-macro" "-Wno-unused-macros" "-Wno-switch-enum")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsEXE "-Wno-unsafe-buffer-usage")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-attributes" "-Walloc-size-larger-than=4GB")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /Zc:inline)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus /Zc:inline)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT ((${DIRECTX_ARCH} MATCHES "^arm"))))
      target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()

    if(BUILD_FUZZING
       AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32))
          target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=fuzzer /fsanitize=address /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
          target_link_libraries(${PROJECT_NAME} PRIVATE sancov.lib)
          target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4291)
    endif()

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4668" "/wd4710" "/wd4820" "/wd5031" "/wd5032" "/wd5039" "/wd5045" )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE "/wd5262" "/wd5264")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})
endif()
