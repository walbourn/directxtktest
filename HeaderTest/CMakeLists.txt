cmake_minimum_required (VERSION 3.20)

project (headertest
  DESCRIPTION "DirectX Tool Kit for DX11 Header Test"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "DirectX Tool Kit Test Suite should be built by the main CMakeLists")
endif()

add_executable(headertest
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
    main.cpp
    modeltest.cpp
    mousetest.cpp
    postprocesstest.cpp
    primitivebatchtest.cpp
    screengrabtest.cpp
    simplemathtest.cpp
    spritebatchtest.cpp
    spritefonttest.cpp
    vertextypestest.cpp
    wictextureloadertest.cpp
    )

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(headertest PRIVATE /wd4061 /wd4365 /wd4514 /wd4668 /wd4820 /wd5039)
endif()
