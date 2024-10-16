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

if((BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7) AND (NOT NO_WCHAR_T))
  # wavtest
  list(APPEND TEST_EXES wavtest)
  list(APPEND XAUDIO_TESTS wavtest)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/WavTest)
  add_test(NAME "wavtest" COMMAND wavtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(wavtest PROPERTIES LABELS "Audio")
  set_tests_properties(wavtest PROPERTIES TIMEOUT 30)

  # fuzzloaders
  list(APPEND TEST_EXES fuzzloaders)
  list(APPEND XAUDIO_TESTS fuzzloaders)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/fuzzloaders)
endif()

# ddswictest
list(APPEND TEST_EXES ddswictest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/DdsWicTest)
add_test(NAME "ddswictest" COMMAND ddswictest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(ddswictest PROPERTIES LABELS "ImageFormats")
set_tests_properties(ddswictest PROPERTIES TIMEOUT 60)

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
if(NOT MINGW)
  target_precompile_headers(d3d11test PRIVATE D3D11Test/pch.h)
endif()
add_test(NAME "d3d11" COMMAND d3d11test -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/D3D11Test)
set_tests_properties(d3d11 PROPERTIES LABELS "Graphics")
set_tests_properties(d3d11 PROPERTIES TIMEOUT 30)

# MSAA
list(APPEND TEST_EXES msaatest)
add_executable(msaatest WIN32
    MSAATest/Game.cpp
    MSAATest/Game.h
    MSAATest/pch.h
    MSAATest/MSAAHelper.cpp
    MSAATest/MSAAHelper.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(msaatest PRIVATE ./MSAATest ../Src)
if(NOT MINGW)
  target_precompile_headers(msaatest PRIVATE MSAATest/pch.h)
endif()
add_test(NAME "msaa" COMMAND msaatest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MSAATest)
set_tests_properties(msaa PROPERTIES LABELS "Graphics")
set_tests_properties(msaa PROPERTIES TIMEOUT 60)

# EFFECTS
list(APPEND TEST_EXES effectstest)
add_executable(effectstest WIN32
    EffectsTest/Game.cpp
    EffectsTest/Game.h
    EffectsTest/pch.h
    ${D3D_COMMON_FILES}
    )
target_include_directories(effectstest PRIVATE ./EffectsTest ../Src)
if(NOT MINGW)
  target_precompile_headers(effectstest PRIVATE EffectsTest/pch.h)
endif()
add_test(NAME "effects" COMMAND effectstest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/EffectsTest)
set_tests_properties(effects PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(primitivestest PRIVATE PrimitivesTest/pch.h)
endif()
add_test(NAME "primitiveBatch" COMMAND primitivestest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PrimitivesTest)
set_tests_properties(primitiveBatch PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(spritebatchtest PRIVATE SpriteBatchTest/pch.h)
endif()
add_test(NAME "spriteBatch" COMMAND spritebatchtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatchTest)
set_tests_properties(spriteBatch PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(spritefonttest PRIVATE SpriteFontTest/pch.h)
endif()
if(NO_WCHAR_T)
  target_compile_definitions(spritefonttest PRIVATE NO_WCHAR_T)
endif()
add_test(NAME "spriteFont" COMMAND spritefonttest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SpriteFontTest)
set_tests_properties(spriteFont PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(loadtest PRIVATE LoadTest/pch.h)
endif()
add_test(NAME "loaders" COMMAND loadtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/LoadTest)
set_tests_properties(loaders PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(modeltest PRIVATE ModelTest/pch.h)
endif()
add_test(NAME "models" COMMAND modeltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ModelTest)
set_tests_properties(models PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(shadertest PRIVATE ShaderTest/pch.h)
endif()
add_test(NAME "shaders" COMMAND shadertest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ShaderTest)
set_tests_properties(shaders PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(animtest PRIVATE AnimTest/pch.h)
endif()
add_test(NAME "animation" COMMAND animtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AnimTest)
set_tests_properties(animation PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(dgsltest PRIVATE DGSLTest/pch.h)
endif()
add_test(NAME "dgslShaders" COMMAND dgsltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DGSLTest)
set_tests_properties(dgslShaders PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(hdrtest PRIVATE HDRTest/pch.h)
endif()
add_test(NAME "hdr" COMMAND hdrtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/HDRTest)
set_tests_properties(hdr PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(pbrtest PRIVATE PBRTest/pch.h)
endif()
add_test(NAME "pbrShaders" COMMAND pbrtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PBRTest)
set_tests_properties(pbrShaders PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(pbrmodeltest PRIVATE PBRModelTest/pch.h)
endif()
add_test(NAME "modelsPBR" COMMAND pbrmodeltest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PBRModelTest)
set_tests_properties(modelsPBR PROPERTIES LABELS "Graphics")
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
if(NOT MINGW)
  target_precompile_headers(postprocesstest PRIVATE PostProcessTest/pch.h)
endif()
add_test(NAME "postProcess" COMMAND postprocesstest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessTest)
set_tests_properties(postProcess PROPERTIES LABELS "Graphics")
set_tests_properties(postProcess PROPERTIES TIMEOUT 180)

# MATH
list(APPEND TEST_EXES simplemathtest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SimpleMathTest)
add_test(NAME "simplemath" COMMAND simplemathtest)
set_tests_properties(simplemath PROPERTIES LABELS "Math")
set_tests_properties(simplemath PROPERTIES TIMEOUT 10)

if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
    # BASIC AUDIO
    list(APPEND TEST_EXES basicaudiotest)
    list(APPEND XAUDIO_TESTS basicaudiotest)
    add_executable(basicaudiotest BasicAudioTest/BasicAudioTest.cpp)
    add_test(NAME "basicAudio" COMMAND basicaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BasicAudioTest)
    set_tests_properties(basicAudio PROPERTIES LABELS "Audio")
    set_tests_properties(basicAudio PROPERTIES TIMEOUT 800)

    # DYNAMIC AUDIO (Media Foundation streaming)
    if(NOT MINGW)
        list(APPEND TEST_EXES dynamicaudiotest)
        list(APPEND XAUDIO_TESTS dynamicaudiotest)
        add_executable(dynamicaudiotest DynamicAudioTest/DynamicAudioTest.cpp)
        add_test(NAME "dynamicAudio" COMMAND dynamicaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DynamicAudioTest)
        set_tests_properties(dynamicAudio PROPERTIES LABELS "Audio")
        set_tests_properties(dynamicAudio PROPERTIES TIMEOUT 270)
    endif()

    # STREAMING AUDIO
    list(APPEND TEST_EXES streamingaudiotest)
    list(APPEND XAUDIO_TESTS streamingaudiotest)
    add_executable(streamingaudiotest StreamingAudioTest/StreamingAudioTest.cpp)
    add_test(NAME "streamingAudio" COMMAND streamingaudiotest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/StreamingAudioTest)
    set_tests_properties(streamingAudio PROPERTIES LABELS "Audio")
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
    if(NOT MINGW)
      target_precompile_headers(audio3dtest PRIVATE Audio3DTest/pch.h)
    endif()
    add_test(NAME "positionalAudio" COMMAND audio3dtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Audio3DTest)
    set_tests_properties(positionalAudio PROPERTIES LABELS "Audio")
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
    if(NOT MINGW)
      target_precompile_headers(simpleaudiotest PRIVATE SimpleAudioTest/pch.h)
    endif()
    add_test(NAME "simpleAudio" COMMAND simpleaudiotest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAudioTest)
    set_tests_properties(simpleAudio PROPERTIES LABELS "Audio")
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
if(NOT MINGW)
  target_precompile_headers(gamepadtest PRIVATE GamePadTest/pch.h)
endif()
add_test(NAME "gamepadInput" COMMAND gamepadtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/GamePadTest)
set_tests_properties(gamepadInput PROPERTIES LABELS "Input")
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
if(NOT MINGW)
  target_precompile_headers(keyboardtest PRIVATE KeyboardTest/pch.h)
endif()
add_test(NAME "keyboardInput" COMMAND keyboardtest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/KeyboardTest)
set_tests_properties(keyboardInput PROPERTIES LABELS "Input")
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
if(NOT MINGW)
  target_precompile_headers(mousetest PRIVATE MouseTest/pch.h)
endif()
add_test(NAME "mouseInput" COMMAND mousetest -ctest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MouseTest)
set_tests_properties(mouseInput PROPERTIES LABELS "Input")
set_tests_properties(mouseInput PROPERTIES TIMEOUT 30)

message(STATUS "Enabled tests: ${TEST_EXES}")
foreach(t IN LISTS TEST_EXES)
  target_include_directories(${t} PRIVATE ./Common)
  target_link_libraries(${t} PRIVATE DirectXTK dxgi.lib d3d11.lib)
endforeach()

if(directxmath_FOUND)
    foreach(t IN LISTS TEST_EXES)
      target_link_libraries(${t} PRIVATE Microsoft::DirectXMath)
    endforeach()
endif()

if(xaudio2redist_FOUND
   AND BUILD_XAUDIO_WIN7)
    foreach(t IN LISTS XAUDIO_TESTS)
      target_link_libraries(${t} PRIVATE Microsoft::XAudio2Redist)
      target_compile_definitions(${t} PRIVATE USING_XAUDIO2_REDIST)
    endforeach()
endif()

if(BUILD_GAMEINPUT)
    foreach(t IN LISTS TEST_EXES)
      target_compile_definitions(${t} PRIVATE USING_GAMEINPUT)
    endforeach()
elseif(BUILD_WGI)
    foreach(t IN LISTS TEST_EXES)
      target_compile_definitions(${t} PRIVATE USING_WINDOWS_GAMING_INPUT)
    endforeach()
elseif(BUILD_XINPUT)
    foreach(t IN LISTS TEST_EXES)
      target_compile_definitions(${t} PRIVATE USING_XINPUT)
    endforeach()
endif()

if(MSVC)
    foreach(t IN LISTS TEST_EXES)
      # Model requires RTTI (/GR)
      target_compile_options(${t} PRIVATE "$<IF:$<STREQUAL:${t},headertest>,/Wall,/W4>" /EHsc /GR)
    endforeach()
endif()

if(DEFINED COMPILER_DEFINES)
    foreach(t IN LISTS TEST_EXES)
      target_compile_definitions(${t} PRIVATE ${COMPILER_DEFINES})
      target_compile_options(${t} PRIVATE ${COMPILER_SWITCHES})
      target_link_options(${t} PRIVATE ${LINKER_SWITCHES})
    endforeach()
endif()

if(MINGW)
    foreach(t IN LISTS TEST_EXES)
      target_link_options(${t} PRIVATE -municode)
      target_link_libraries(${t} PRIVATE "$<IF:$<STREQUAL:${WINVER},0x0601>,xinput9_1_0.lib,xinput1_4.lib>")
    endforeach()
    foreach(t IN LISTS XAUDIO_TESTS)
      target_link_libraries(${t} PRIVATE ksguid.lib)
    endforeach()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    target_compile_options(headertest PRIVATE -Wall -Wpedantic -Wextra)

    set(WarningsEXE "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-language-extension-token" "-Wno-reserved-macro-identifier")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsEXE "-Wno-unsafe-buffer-usage")
    endif()
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    foreach(t IN LISTS TEST_EXES)
      target_compile_options(${t} PRIVATE "-Wno-ignored-attributes" "-Walloc-size-larger-than=4GB")
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    foreach(t IN LISTS TEST_EXES)
      set_target_properties(${t} PROPERTIES CXX_STANDARD 14)
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(ENABLE_CODE_ANALYSIS)
      foreach(t IN LISTS TEST_EXES)
        target_compile_options(${t} PRIVATE /analyze /WX)
      endforeach()
    endif()

    if(BUILD_FUZZING
       AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32))
        foreach(t IN LISTS TEST_EXES)
          target_compile_options(${t} PRIVATE ${ASAN_SWITCHES})
          target_link_libraries(${t} PRIVATE ${ASAN_LIBS})
        endforeach()
    endif()
endif()

if(WIN32)
    foreach(t IN LISTS TEST_EXES)
      target_compile_definitions(${t} PRIVATE _WIN32_WINNT=${WINVER})
    endforeach()

    if(DISABLE_MSVC_ITERATOR_DEBUGGING)
      foreach(t IN LISTS TEST_EXES)
        target_compile_definitions(${t} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
      endforeach()
    endif()
endif()

if(ENABLE_CODE_COVERAGE)
  foreach(t IN LISTS TEST_EXES)
    if(DEFINED COV_COMPILER_SWITCHES)
      target_compile_options(${t} PRIVATE ${COV_COMPILER_SWITCHES})
    endif()
    if(DEFINED COV_LINKER_SWITCHES)
      target_link_options(${t} PRIVATE ${COV_LINKER_SWITCHES})
    endif()
    if(DEFINED COV_LIBS)
      target_link_libraries(${t} PRIVATE ${COV_LIBS})
    endif()
  endforeach()

  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/MSVCCodeCov.cmd.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MSVCCodeCov.cmd COPYONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/coverage.config.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/coverage.config COPYONLY)

    configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/MSVCCodeCovInput.cmd.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MSVCCodeCovInput.cmd COPYONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/coverage-input.config.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/coverage-input.config COPYONLY)

    if(BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
      configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/MSVCCodeCovAudio.cmd.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MSVCCodeCovAudio.cmd COPYONLY)
      configure_file(${CMAKE_CURRENT_LIST_DIR}/codecov/coverage-audio.config.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/coverage-audio.config COPYONLY)
    endif()
  endif()
endif()
