# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (DirectXTKTest
  DESCRIPTION "DirectX Tool Kit for DX11 Test Suite"
  HOMEPAGE_URL "https://github.com/walbourn/directxtktest/wiki"
  LANGUAGES CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "DirectX Tool Kit Test Suite should be built by the main CMakeLists")
endif()

set(TEST_EXES headertest)
set(XAUDIO_TESTS headertest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/HeaderTest)

# fuzzloaders
if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
  list(APPEND TEST_EXES fuzzloaders)
  list(APPEND XAUDIO_TESTS fuzzloaders)
  add_executable(fuzzloaders fuzzloaders/fuzzloaders.cpp)
  target_include_directories(fuzzloaders PRIVATE ../Audio)
endif()

# D3D11
set(D3D_COMMON_FILES
    Common/MainPC.cpp
    Common/DeviceResourcesPC.cpp
    Common/DeviceResourcesPC.h
    Common/DirectXTKTest.h
    Common/StepTimer.h
    )

list(APPEND TEST_EXES d3d11test)
add_executable(d3d11test WIN32
    D3D11Test/Game.cpp
    D3D11Test/Game.h
    D3D11Test/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(d3d11test PRIVATE ./D3D11Test)
if (NOT MINGW)
  target_precompile_headers(d3d11test PRIVATE D3D11Test/pch.h)
endif()
add_test(NAME "d3d11" COMMAND d3d11test -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/D3D11Test)
set_tests_properties(d3d11 PROPERTIES TIMEOUT 30)

# EFFECTS
list(APPEND TEST_EXES effectstest)
add_executable(effectstest WIN32
    EffectsTest/Game.cpp
    EffectsTest/Game.h
    EffectsTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(effectstest PRIVATE ./EffectsTest ../Src)
if (NOT MINGW)
  target_precompile_headers(effectstest PRIVATE EffectsTest/pch.h)
endif()
add_test(NAME "effects" COMMAND effectstest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/EffectsTest)
set_tests_properties(effects PROPERTIES TIMEOUT 60)

# PRIMITIVE BATCH
list(APPEND TEST_EXES primitivestest)
add_executable(primitivestest WIN32
    PrimitivesTest/Game.cpp
    PrimitivesTest/Game.h
    PrimitivesTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(primitivestest PRIVATE ./PrimitivesTest)
if (NOT MINGW)
  target_precompile_headers(primitivestest PRIVATE PrimitivesTest/pch.h)
endif()
add_test(NAME "primitiveBatch" COMMAND primitivestest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PrimitivesTest)
set_tests_properties(primitiveBatch PROPERTIES TIMEOUT 60)

# SPRITE BATCH
list(APPEND TEST_EXES spritebatchtest)
add_executable(spritebatchtest WIN32
    SpriteBatchTest/Game.cpp
    SpriteBatchTest/Game.h
    SpriteBatchTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(spritebatchtest PRIVATE ./SpriteBatchTest)
if (NOT MINGW)
  target_precompile_headers(spritebatchtest PRIVATE SpriteBatchTest/pch.h)
endif()
add_test(NAME "spriteBatch" COMMAND spritebatchtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatchTest)
set_tests_properties(spriteBatch PROPERTIES TIMEOUT 60)

# SPRITE FONT
list(APPEND TEST_EXES spritefonttest)
add_executable(spritefonttest WIN32
    SpriteFontTest/Game.cpp
    SpriteFontTest/Game.h
    SpriteFontTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(spritefonttest PRIVATE ./SpriteFontTest)
if (NOT MINGW)
  target_precompile_headers(spritefonttest PRIVATE SpriteFontTest/pch.h)
endif()
add_test(NAME "spriteFont" COMMAND spritefonttest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SpriteFontTest)
set_tests_properties(spriteFont PROPERTIES TIMEOUT 60)

# LOAD TEST
list(APPEND TEST_EXES loadtest)
add_executable(loadtest WIN32
    LoadTest/Game.cpp
    LoadTest/Game.h
    LoadTest/pch.h
    Common/ReadData.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(loadtest PRIVATE ./LoadTest)
if (NOT MINGW)
  target_precompile_headers(loadtest PRIVATE LoadTest/pch.h)
endif()
add_test(NAME "loaders" COMMAND loadtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/LoadTest)
set_tests_properties(loaders PROPERTIES TIMEOUT 30)

# MODEL TEST
list(APPEND TEST_EXES modeltest)
add_executable(modeltest WIN32
    ModelTest/Game.cpp
    ModelTest/Game.h
    ModelTest/ModelLoadOBJ.cpp
    ModelTest/pch.h
    ModelTest/WaveFrontReader.h
    Common/ReadData.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(modeltest PRIVATE ./ModelTest)
if (NOT MINGW)
  target_precompile_headers(modeltest PRIVATE ModelTest/pch.h)
endif()
add_test(NAME "models" COMMAND modeltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ModelTest)
set_tests_properties(models PROPERTIES TIMEOUT 30)

# SHADER TEST
list(APPEND TEST_EXES shadertest)
add_executable(shadertest WIN32
    ShaderTest/Game.cpp
    ShaderTest/Game.h
    ShaderTest/pch.h
    Common/RenderTexture.cpp
    Common/RenderTexture.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(shadertest PRIVATE ./ShaderTest)
if (NOT MINGW)
  target_precompile_headers(shadertest PRIVATE ShaderTest/pch.h)
endif()
add_test(NAME "shaders" COMMAND shadertest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ShaderTest)
set_tests_properties(shaders PROPERTIES TIMEOUT 30)

# TODO: AnimTest
# TODO: DGSLTest
# TODO: HDRTest
# TODO: PBRTest
# TODO: PBRModelTest
# TODO: PostProcessTest

# MATH
list(APPEND TEST_EXES simplemathtest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SimpleMathTest)
add_test(NAME "simplemath" COMMAND simplemathtest)
set_tests_properties(simplemath PROPERTIES TIMEOUT 10)

# if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
# TODO: BasicAudioTest
# TODO: DynamicAudioTest
# TODO: Audio3DTest
# TODO: StreamingAudioTest
# TODO: SimpleAudioTest

# TODO: GamePadTest
# TODO: KeyboardTest
# TOOD: MouseTest

message("INFO: Enabled tests: ${TEST_EXES}")
foreach(t IN LISTS TEST_EXES)
  target_include_directories(${t} PRIVATE ./Common)
  target_link_libraries(${t} PRIVATE DirectXTK dxgi.lib d3d11.lib)
endforeach()

if (MINGW OR (NOT WIN32) OR VCPKG_TOOLCHAIN)
    foreach(t IN LISTS TEST_EXES)
      target_link_libraries(${t} PRIVATE Microsoft::DirectXMath)
    endforeach()
    if (BUILD_XAUDIO_WIN7)
      foreach(t IN LISTS XAUDIO_TESTS)
        target_compile_definitions(${t} PRIVATE USING_XAUDIO2_REDIST)
        target_link_libraries(${t} PRIVATE Microsoft::XAudio2Redist)
      endforeach()
    endif()
endif()

if (MSVC)
  foreach(t IN LISTS TEST_EXES)
    # Model requires RTTI (/GR)
    target_compile_options(${t} PRIVATE "$<IF:$<STREQUAL:${t},headertest>,/Wall,/W4>" /GR /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${t} PRIVATE /DYNAMICBASE /NXCOMPAT)
  endforeach()

  if((${CMAKE_SIZEOF_VOID_P} EQUAL 4) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
    foreach(t IN LISTS TEST_EXES)
      target_link_options(${t} PRIVATE /SAFESEH)
    endforeach()
  endif()

  if((MSVC_VERSION GREATER_EQUAL 1928) AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
    AND NOT ENABLE_OPENEXR_SUPPORT
    AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
        target_link_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      endforeach()
    endif()
endif()

if(NOT (${DIRECTX_ARCH} MATCHES "^arm"))
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE ${ARCH_SSE2})
    endforeach()
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options(headertest PRIVATE -Wall -Wpedantic -Wextra)
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token")
    endforeach()
elseif(MINGW)
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE -Wno-ignored-attributes)
      target_compile_definitions(${t} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
      target_link_options(${t} PRIVATE -municode)
      target_link_libraries(${t} PRIVATE xinput.lib)
    endforeach()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus)
    endforeach()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE /ZH:SHA_256)
      endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE /Zc:preprocessor /wd5105)
      endforeach()
    endif()

    if ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT ((${DIRECTX_ARCH} MATCHES "^arm"))))
      foreach(t IN LISTS TEST_EXES)
        target_link_options(${t} PRIVATE /CETCOMPAT)
      endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE /wd5262 /wd5264)
      endforeach()
    endif()
endif()

if(WIN32)
  foreach(t IN LISTS TEST_EXES)
    target_compile_definitions(${t} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})
  endforeach()
endif()
