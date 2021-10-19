# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)

set(DIRECTXTK_VERSION 1.8.0)

project (DirectXTK
  VERSION ${DIRECTXTK_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 11"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkId=248929"
  LANGUAGES CXX)

option(BUILD_TOOLS "Build XWBTool" ON)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" OFF)
option(BUILD_XAUDIO_WIN8 "Build for XAudio 2.8" ON)
option(BUILD_XAUDIO_WIN7 "Build for XAudio2Redist" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

#--- Library
set(LIBRARY_HEADERS
    Inc/BufferHelpers.h
    Inc/CommonStates.h
    Inc/DDSTextureLoader.h
    Inc/DirectXHelpers.h
    Inc/Effects.h
    Inc/GamePad.h
    Inc/GeometricPrimitive.h
    Inc/GraphicsMemory.h
    Inc/Keyboard.h
    Inc/Model.h
    Inc/Mouse.h
    Inc/PostProcess.h
    Inc/PrimitiveBatch.h
    Inc/ScreenGrab.h
    Inc/SimpleMath.h
    Inc/SimpleMath.inl
    Inc/SpriteBatch.h
    Inc/SpriteFont.h
    Inc/VertexTypes.h
    Inc/WICTextureLoader.h)

set(LIBRARY_SOURCES
    Src/AlignedNew.h
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/Bezier.h
    Src/BinaryReader.cpp
    Src/BinaryReader.h
    Src/BufferHelpers.cpp
    Src/CommonStates.cpp
    Src/DDS.h
    Src/DDSTextureLoader.cpp
    Src/DebugEffect.cpp
    Src/DemandCreate.h
    Src/DGSLEffect.cpp
    Src/DGSLEffectFactory.cpp
    Src/DirectXHelpers.cpp
    Src/DualPostProcess.cpp
    Src/DualTextureEffect.cpp
    Src/EffectCommon.cpp
    Src/EffectCommon.h
    Src/EffectFactory.cpp
    Src/EnvironmentMapEffect.cpp
    Src/GamePad.cpp
    Src/GeometricPrimitive.cpp
    Src/Geometry.h
    Src/Geometry.cpp
    Src/GraphicsMemory.cpp
    Src/Keyboard.cpp
    Src/LoaderHelpers.h
    Src/Model.cpp
    Src/ModelLoadCMO.cpp
    Src/ModelLoadSDKMESH.cpp
    Src/ModelLoadVBO.cpp
    Src/Mouse.cpp
    Src/NormalMapEffect.cpp
    Src/PBREffect.cpp
    Src/PBREffectFactory.cpp
    Src/pch.h
    Src/PlatformHelpers.h
    Src/PrimitiveBatch.cpp
    Src/ScreenGrab.cpp
    Src/SDKMesh.h
    Src/SharedResourcePool.h
    Src/SimpleMath.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/TeapotData.inc
    Src/ToneMapPostProcess.cpp
    Src/vbo.h
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp)

set(SHADER_SOURCES
    Src/Shaders/AlphaTestEffect.fx
    Src/Shaders/BasicEffect.fx
    Src/Shaders/Common.fxh
    Src/Shaders/DebugEffect.fx
    Src/Shaders/DGSLEffect.fx
    Src/Shaders/DGSLLambert.hlsl
    Src/Shaders/DGSLPhong.hlsl
    Src/Shaders/DGSLUnlit.hlsl
    Src/Shaders/DualTextureEffect.fx
    Src/Shaders/EnvironmentMapEffect.fx
    Src/Shaders/Lighting.fxh
    Src/Shaders/NormalMapEffect.fx
    Src/Shaders/PBRCommon.fxh
    Src/Shaders/PBREffect.fx
    Src/Shaders/PixelPacking_Velocity.hlsli
    Src/Shaders/PostProcess.fx
    Src/Shaders/SkinnedEffect.fx
    Src/Shaders/Skinning.fxh
    Src/Shaders/SpriteEffect.fx
    Src/Shaders/Structures.fxh
    Src/Shaders/ToneMap.fx
    Src/Shaders/Utilities.fxh)

if((BUILD_XAUDIO_WIN10) OR (BUILD_XAUDIO_WIN8) OR (BUILD_XAUDIO_WIN7))
    set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
        Inc/Audio.h)

    set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
        Audio/AudioEngine.cpp
        Audio/DynamicSoundEffectInstance.cpp
        Audio/SoundCommon.cpp
        Audio/SoundCommon.h
        Audio/SoundEffect.cpp
        Audio/SoundEffectInstance.cpp
        Audio/SoundStreamInstance.cpp
        Audio/WaveBank.cpp
        Audio/WaveBankReader.cpp
        Audio/WaveBankReader.h
        Audio/WAVFileReader.cpp
        Audio/WAVFileReader.h)
endif()

if (NOT COMPILED_SHADERS)
    if (USE_PREBUILT_SHADERS)
        message(FATAL_ERROR "ERROR: Using prebuilt shaders requires the COMPILED_SHADERS variable is set")
    endif()
    set(COMPILED_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/Shaders/Compiled)
    file(MAKE_DIRECTORY ${COMPILED_SHADERS})
endif()

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc)

if (NOT USE_PREBUILT_SHADERS)
    add_custom_command(
        OUTPUT "${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc"
        MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Generating HLSL shaders..."
        COMMAND set CompileShadersOutput=${COMPILED_SHADERS}
        COMMAND CompileShaders.cmd
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Src/Shaders"
        USES_TERMINAL)
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${COMPILED_SHADERS})

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)

if((BUILD_XAUDIO_WIN10) OR (BUILD_XAUDIO_WIN8) OR (BUILD_XAUDIO_WIN7))
    target_include_directories(${PROJECT_NAME} PRIVATE Src Audio)
endif()

if(MSVC)
    # Use max Warning Level
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Model uses dynamic_cast, so we need /GR (Enable RTTI)

    # Library needs /EHsc (Enable C++ exceptions)
endif()

if (VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectXMath.")
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXMath)
endif()

if (BUILD_XAUDIO_WIN7 AND (NOT BUILD_XAUDIO_WIN10) AND (NOT BUILD_XAUDIO_WIN8) AND (NOT WINDOWS_STORE))
    message("INFO: Using XAudio2Redist for DirectX Tool Kit for Audio on Windows 7.")
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_XAUDIO2_REDIST)

    find_package(xaudio2redist CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::XAudio2Redist)
endif()

#--- Package
include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)

write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
  VERSION ${DIRECTXTK_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/.nuget/${PROJECT_NAME}-config.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
  INSTALL_DESTINATION cmake/})

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Microsoft::
  DESTINATION cmake/)

install(FILES ${LIBRARY_HEADERS}
    DESTINATION include/${PROJECT_NAME})

install(FILES
    ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
    ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
  DESTINATION cmake/)

#--- Command-line tools
if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
  add_executable(xwbtool
    xwbtool/xwbtool.cpp
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h)
  target_include_directories(xwbtool PRIVATE Audio Src)
  target_link_libraries(xwbtool version.lib)
  source_group(xwbtool REGULAR_EXPRESSION XWBTool/*.*)

  if (VCPKG_TOOLCHAIN)
    target_link_libraries(xwbtool Microsoft::DirectXMath)
  endif()
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)
    if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
      target_compile_options(xwbtool PRIVATE /fp:fast)
    endif()

   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
      if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
        target_compile_options(xwbtool PRIVATE /analyze)
      endif()
   endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5104 /wd5105)
        if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
          target_compile_options(xwbtool PRIVATE /Zc:preprocessor /wd5104 /wd5105)
        endif()
    endif()

    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4" AND (NOT ${CMAKE_VS_PLATFORM_NAME} MATCHES "arm"))
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
        if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
          target_compile_options(xwbtool PRIVATE /arch:SSE2)
        endif()
    endif()
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsLib "-Wpedantic" "-Wextra")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
      set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-gnu-anonymous-struct"
        "-Wno-missing-prototypes" "-Wno-nested-anon-types" "-Wno-unused-const-variable")
      target_compile_options(xwbtool PRIVATE ${WarningsEXE})
    endif()
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus)

    if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
      target_compile_options(xwbtool PRIVATE /permissive- /Zc:__cplusplus)

      set(WarningsEXE "/wd4365" "/wd4514" "/wd4625" "/wd4626" "/wd4627" "/wd4710" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045")
      target_compile_options(xwbtool PRIVATE ${WarningsEXE})
    endif()
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

    if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
      target_compile_definitions(xwbtool PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0601)
    endif()

    if(BUILD_XAUDIO_WIN10 OR WINDOWS_STORE)
        if(BUILD_XAUDIO_WIN10)
            message("INFO: Using DirectX Tool Kit for Audio on XAudio 2.9 (Windows 10).")
        endif()
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    elseif(BUILD_XAUDIO_WIN8)
        message("INFO: Using DirectX Tool Kit for Audio on XAudio 2.8 (Windows 8).")
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0602)
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
    endif()
endif()

if(BUILD_TOOLS AND (NOT WINDOWS_STORE))
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT xwbtool)
endif()
