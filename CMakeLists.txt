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

# DDS/WIC TEXTURE LOADER
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

# MODEL
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
set_tests_properties(models PROPERTIES TIMEOUT 60)

# SHADER VALIDATION
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
set_tests_properties(shaders PROPERTIES TIMEOUT 40)

# ANIMATION
list(APPEND TEST_EXES animtest)
add_executable(animtest WIN32
    AnimTest/Game.cpp
    AnimTest/Game.h
    AnimTest/pch.h
    Common/Animation.cpp
    Common/Animation.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(animtest PRIVATE ./AnimTest)
if (NOT MINGW)
  target_precompile_headers(animtest PRIVATE AnimTest/pch.h)
endif()
add_test(NAME "animation" COMMAND animtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AnimTest)
set_tests_properties(animation PROPERTIES TIMEOUT 40)

# DGSL
list(APPEND TEST_EXES dgsltest)
add_executable(dgsltest WIN32
    DGSLTest/Game.cpp
    DGSLTest/Game.h
    DGSLTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(dgsltest PRIVATE ./DGSLTest)
if (NOT MINGW)
  target_precompile_headers(dgsltest PRIVATE DGSLTest/pch.h)
endif()
add_test(NAME "dgslShaders" COMMAND dgsltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DGSLTest)
set_tests_properties(dgslShaders PROPERTIES TIMEOUT 40)

# HDR
list(APPEND TEST_EXES hdrtest)
add_executable(hdrtest WIN32
    HDRTest/Game.cpp
    HDRTest/Game.h
    HDRTest/pch.h
    Common/RenderTexture.cpp
    Common/RenderTexture.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(hdrtest PRIVATE ./HDRTest)
if (NOT MINGW)
  target_precompile_headers(hdrtest PRIVATE HDRTest/pch.h)
endif()
add_test(NAME "hdr" COMMAND hdrtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/HDRTest)
set_tests_properties(hdr PROPERTIES TIMEOUT 40)

# PBR
list(APPEND TEST_EXES pbrtest)
add_executable(pbrtest WIN32
    PBRTest/Game.cpp
    PBRTest/Game.h
    PBRTest/pch.h
    Common/RenderTexture.cpp
    Common/RenderTexture.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(pbrtest PRIVATE ./PBRTest ../Src)
if (NOT MINGW)
  target_precompile_headers(pbrtest PRIVATE PBRTest/pch.h)
endif()
add_test(NAME "pbrShaders" COMMAND pbrtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PBRTest)
set_tests_properties(pbrShaders PROPERTIES TIMEOUT 40)

# PBR MODEL
list(APPEND TEST_EXES pbrmodeltest)
add_executable(pbrmodeltest WIN32
    PBRModelTest/Game.cpp
    PBRModelTest/Game.h
    PBRModelTest/pch.h
    Common/Animation.cpp
    Common/Animation.h
    Common/RenderTexture.cpp
    Common/RenderTexture.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(pbrmodeltest PRIVATE ./PBRModelTest)
if (NOT MINGW)
  target_precompile_headers(pbrmodeltest PRIVATE PBRModelTest/pch.h)
endif()
add_test(NAME "modelsPBR" COMMAND pbrmodeltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PBRModelTest)
set_tests_properties(modelsPBR PROPERTIES TIMEOUT 60)

# POST-PROCESSING
list(APPEND TEST_EXES postprocesstest)
add_executable(postprocesstest WIN32
    PostProcessTest/Game.cpp
    PostProcessTest/Game.h
    PostProcessTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(postprocesstest PRIVATE ./PostProcessTest)
if (NOT MINGW)
  target_precompile_headers(postprocesstest PRIVATE PostProcessTest/pch.h)
endif()
add_test(NAME "postProcess" COMMAND postprocesstest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessTest)
set_tests_properties(postProcess PROPERTIES TIMEOUT 180)

# MATH
list(APPEND TEST_EXES simplemathtest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SimpleMathTest)
add_test(NAME "simplemath" COMMAND simplemathtest)
set_tests_properties(simplemath PROPERTIES TIMEOUT 10)

if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
    # BASIC AUDIO
    list(APPEND TEST_EXES basicaudiotest)
    list(APPEND XAUDIO_TESTS basicaudiotest)
    add_executable(basicaudiotest BasicAudioTest/BasicAudioTest.cpp)
    add_test(NAME "basicAudio" COMMAND basicaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BasicAudioTest)
    set_tests_properties(basicAudio PROPERTIES TIMEOUT 800)

    # DYNAMIC AUDIO (Media Foundation streaming)
    if(NOT MINGW)
        list(APPEND TEST_EXES dynamicaudiotest)
        list(APPEND XAUDIO_TESTS dynamicaudiotest)
        add_executable(dynamicaudiotest DynamicAudioTest/DynamicAudioTest.cpp)
        add_test(NAME "dynamicAudio" COMMAND dynamicaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DynamicAudioTest)
        set_tests_properties(dynamicAudio PROPERTIES TIMEOUT 270)
    endif()

    # STREAMING AUDIO
    list(APPEND TEST_EXES streamingaudiotest)
    list(APPEND XAUDIO_TESTS streamingaudiotest)
    add_executable(streamingaudiotest StreamingAudioTest/StreamingAudioTest.cpp)
    add_test(NAME "streamingAudio" COMMAND streamingaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/StreamingAudioTest)
    set_tests_properties(streamingAudio PROPERTIES TIMEOUT 800)

    # 3D AUDIO
    list(APPEND TEST_EXES audio3dtest)
    list(APPEND XAUDIO_TESTS audio3dtest)
    add_executable(audio3dtest WIN32
        Audio3DTest/Game.cpp
        Audio3DTest/Game.h
        Audio3DTest/pch.h
        ${D3D_COMMON_FILES}
        )
    target_include_directories(audio3dtest PRIVATE ./Audio3DTest)
    if (NOT MINGW)
      target_precompile_headers(audio3dtest PRIVATE Audio3DTest/pch.h)
    endif()
    add_test(NAME "positionalAudio" COMMAND audio3dtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Audio3DTest)
    set_tests_properties(positionalAudio PROPERTIES TIMEOUT 40)

    # SIMPLE AUDIO
    list(APPEND TEST_EXES simpleaudiotest)
    list(APPEND XAUDIO_TESTS simpleaudiotest)
    add_executable(simpleaudiotest WIN32
        SimpleAudioTest/Game.cpp
        SimpleAudioTest/Game.h
        SimpleAudioTest/pch.h
        Common/TextConsole.cpp
        Common/TextConsole.h
        ${D3D_COMMON_FILES}
        )
    target_include_directories(simpleaudiotest PRIVATE ./SimpleAudioTest)
    if (NOT MINGW)
      target_precompile_headers(simpleaudiotest PRIVATE SimpleAudioTest/pch.h)
    endif()
    add_test(NAME "simpleAudio" COMMAND simpleaudiotest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAudioTest)
    set_tests_properties(simpleAudio PROPERTIES TIMEOUT 30)
endif()

# INPUT: GAMEPAD
list(APPEND TEST_EXES gamepadtest)
add_executable(gamepadtest WIN32
    GamePadTest/Game.cpp
    GamePadTest/Game.h
    GamePadTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(gamepadtest PRIVATE ./GamePadTest)
if (NOT MINGW)
  target_precompile_headers(gamepadtest PRIVATE GamePadTest/pch.h)
endif()
add_test(NAME "gamepadInput" COMMAND gamepadtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/GamePadTest)
set_tests_properties(gamepadInput PROPERTIES TIMEOUT 30)

# INPUT: KEYBOARD
list(APPEND TEST_EXES keyboardtest)
add_executable(keyboardtest WIN32
    KeyboardTest/Game.cpp
    KeyboardTest/Game.h
    KeyboardTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(keyboardtest PRIVATE ./KeyboardTest)
if (NOT MINGW)
  target_precompile_headers(keyboardtest PRIVATE KeyboardTest/pch.h)
endif()
add_test(NAME "keyboardInput" COMMAND keyboardtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/KeyboardTest)
set_tests_properties(keyboardInput PROPERTIES TIMEOUT 30)

# INPUT: MOUSE
list(APPEND TEST_EXES mousetest)
add_executable(mousetest WIN32
    MouseTest/Game.cpp
    MouseTest/Game.h
    MouseTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(mousetest PRIVATE ./MouseTest)
if (NOT MINGW)
  target_precompile_headers(mousetest PRIVATE MouseTest/pch.h)
endif()
add_test(NAME "mouseInput" COMMAND mousetest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MouseTest)
set_tests_properties(mouseInput PROPERTIES TIMEOUT 30)

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
      target_link_libraries(${t} PRIVATE "$<IF:$<STREQUAL:${WINVER},0x0601>,xinput9_1_0.lib,xinput1_4.lib>")
    endforeach()
    foreach(t IN LISTS XAUDIO_TESTS)
      target_link_libraries(${t} PRIVATE ksguid.lib)
    endforeach()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus /Zc:inline)
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

    if ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
      foreach(t IN LISTS TEST_EXES)
        target_link_options(${t} PRIVATE /CETCOMPAT)
      endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE /Zc:lambda)
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

    if(DISABLE_MSVC_ITERATOR_DEBUGGING)
      foreach(t IN LISTS TEST_EXES)
        target_compile_definitions(${t} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
      endforeach()
    endif()
endif()
