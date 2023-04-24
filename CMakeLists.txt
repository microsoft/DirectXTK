# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXTK_VERSION 1.8.3)

project (DirectXTK
  VERSION ${DIRECTXTK_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 11"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkId=248929"
  LANGUAGES CXX)

option(BUILD_TOOLS "Build XWBTool" ON)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" OFF)
option(BUILD_XAUDIO_WIN8 "Build for XAudio 2.8" ON)
option(BUILD_XAUDIO_WIN7 "Build for XAudio2Redist" OFF)

option(BUILD_GAMEINPUT "Build for GameInput" OFF)
option(BUILD_WGI "Build for Windows.Gaming.Input" OFF)
option(BUILD_XINPUT "Build for XInput" OFF)

# https://devblogs.microsoft.com/cppblog/spectre-mitigations-in-msvc/
option(ENABLE_SPECTRE_MITIGATION "Build using /Qspectre for MSVC" OFF)

option(DISABLE_MSVC_ITERATOR_DEBUGGING "Disable iterator debugging in Debug configurations with the MSVC CRT" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

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
endif()

include(GNUInstallDirs)

if(WINDOWS_STORE)
   set(BUILD_GAMEINPUT OFF)
   set(USING_WINDOWS_GAMING_INPUT ON)
   set(BUILD_TOOLS OFF)
endif()

#--- Library
set(LIBRARY_HEADERS
    Inc/BufferHelpers.h
    Inc/CommonStates.h
    Inc/DDSTextureLoader.h
    Inc/DirectXHelpers.h
    Inc/Effects.h
    Inc/GeometricPrimitive.h
    Inc/GraphicsMemory.h
    Inc/Model.h
    Inc/PostProcess.h
    Inc/PrimitiveBatch.h
    Inc/ScreenGrab.h
    Inc/SpriteBatch.h
    Inc/SpriteFont.h
    Inc/VertexTypes.h
    Inc/WICTextureLoader.h)

set(LIBRARY_SOURCES
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/BufferHelpers.cpp
    Src/CommonStates.cpp
    Src/DDSTextureLoader.cpp
    Src/DebugEffect.cpp
    Src/DGSLEffect.cpp
    Src/DGSLEffectFactory.cpp
    Src/DirectXHelpers.cpp
    Src/DualPostProcess.cpp
    Src/DualTextureEffect.cpp
    Src/EffectCommon.cpp
    Src/EffectCommon.h
    Src/EffectFactory.cpp
    Src/EnvironmentMapEffect.cpp
    Src/GeometricPrimitive.cpp
    Src/GraphicsMemory.cpp
    Src/Model.cpp
    Src/ModelLoadCMO.cpp
    Src/ModelLoadSDKMESH.cpp
    Src/ModelLoadVBO.cpp
    Src/NormalMapEffect.cpp
    Src/PBREffect.cpp
    Src/PBREffectFactory.cpp
    Src/pch.h
    Src/PrimitiveBatch.cpp
    Src/ScreenGrab.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/ToneMapPostProcess.cpp
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp)

set(SHADER_SOURCES
    Src/Shaders/AlphaTestEffect.fx
    Src/Shaders/BasicEffect.fx
    Src/Shaders/DebugEffect.fx
    Src/Shaders/DGSLEffect.fx
    Src/Shaders/DGSLLambert.hlsl
    Src/Shaders/DGSLPhong.hlsl
    Src/Shaders/DGSLUnlit.hlsl
    Src/Shaders/DualTextureEffect.fx
    Src/Shaders/EnvironmentMapEffect.fx
    Src/Shaders/NormalMapEffect.fx
    Src/Shaders/PBREffect.fx
    Src/Shaders/PostProcess.fx
    Src/Shaders/SkinnedEffect.fx
    Src/Shaders/SpriteEffect.fx
    Src/Shaders/ToneMap.fx)

# These source files are identical in both DX11 and DX12 version.
set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
    Inc/GamePad.h
    Inc/Keyboard.h
    Inc/Mouse.h
    Inc/SimpleMath.h
    Inc/SimpleMath.inl)

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    Src/AlignedNew.h
    Src/Bezier.h
    Src/BinaryReader.h
    Src/DDS.h
    Src/DemandCreate.h
    Src/Geometry.h
    Src/LoaderHelpers.h
    Src/PlatformHelpers.h
    Src/SDKMesh.h
    Src/SharedResourcePool.h
    Src/vbo.h
    Src/TeapotData.inc
    Src/BinaryReader.cpp
    Src/GamePad.cpp
    Src/Geometry.cpp
    Src/Keyboard.cpp
    Src/Mouse.cpp
    Src/SimpleMath.cpp)

set(SHADER_SOURCES ${SHADER_SOURCES}
    Src/Shaders/Common.fxh
    Src/Shaders/Lighting.fxh
    Src/Shaders/PBRCommon.fxh
    Src/Shaders/PixelPacking_Velocity.hlsli
    Src/Shaders/Skinning.fxh
    Src/Shaders/Structures.fxh
    Src/Shaders/Utilities.fxh)

if(MINGW)
   set(BUILD_XAUDIO_WIN10 OFF)
   set(BUILD_XAUDIO_WIN8 OFF)
endif()

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
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

if(NOT COMPILED_SHADERS)
    if(USE_PREBUILT_SHADERS)
        message(FATAL_ERROR "ERROR: Using prebuilt shaders requires the COMPILED_SHADERS variable is set.")
    endif()
    set(COMPILED_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/Shaders/Compiled)
    file(MAKE_DIRECTORY ${COMPILED_SHADERS})
else()
    file(TO_CMAKE_PATH ${COMPILED_SHADERS} COMPILED_SHADERS)
endif()

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc)

if(NOT USE_PREBUILT_SHADERS)
    add_custom_command(
        OUTPUT "${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc"
        MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Generating HLSL shaders..."
        COMMAND ${CMAKE_COMMAND} -E env CompileShadersOutput="${COMPILED_SHADERS}" CompileShaders.cmd > "${COMPILED_SHADERS}/compileshaders.log"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Src/Shaders"
        USES_TERMINAL)
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${COMPILED_SHADERS} Src)

if(NOT MINGW)
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/directxtk>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_WIN7)
    target_include_directories(${PROJECT_NAME} PRIVATE Audio)
endif()

if(MINGW)
    find_package(directxmath CONFIG REQUIRED)
else()
    find_package(directxmath CONFIG QUIET)
endif()

if(directxmath_FOUND)
    message(STATUS "Using DirectXMath package")
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)
endif()

if(BUILD_XAUDIO_WIN7 AND (NOT BUILD_XAUDIO_WIN10) AND (NOT BUILD_XAUDIO_WIN8) AND (NOT WINDOWS_STORE))
    message(STATUS "Using XAudio2Redist for DirectX Tool Kit for Audio on Windows 7.")
    find_package(xaudio2redist CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

include(CheckIncludeFileCXX)

if(BUILD_GAMEINPUT)
    message(STATUS "Using GameInput for GamePad/Keyboard/Mouse.")
    set(CMAKE_REQUIRED_QUIET ON)
    CHECK_INCLUDE_FILE_CXX(GameInput.h GAMEINPUT_HEADER)
    if(NOT GAMEINPUT_HEADER)
        message(FATAL_ERROR "Microsoft GDK required to build GameInput. See https://aka.ms/gdk")
    endif()
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_GAMEINPUT)
elseif(BUILD_WGI)
    message(STATUS "Using Windows.Gaming.Input for GamePad.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_WINDOWS_GAMING_INPUT)
elseif(BUILD_XINPUT)
    message(STATUS "Using XInput for GamePad.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XINPUT)
endif()

#--- Package
include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)

write_basic_package_version_file(
  ${PACKAGE_NAME}-config-version.cmake
  VERSION ${DIRECTXTK_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/build/${PROJECT_NAME}-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Microsoft::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(FILES ${LIBRARY_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/directxtk)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

#--- Command-line tools
if(BUILD_TOOLS AND WIN32)
  set(TOOL_EXES xwbtool)

  add_executable(xwbtool
    xwbtool/xwbtool.cpp
    xwbtool/xwbtool.rc
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h)
  target_include_directories(xwbtool PRIVATE Audio Src)
  target_link_libraries(xwbtool PRIVATE version.lib)
  source_group(xwbtool REGULAR_EXPRESSION XWBTool/*.*)
endif()

if(directxmath_FOUND)
  foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
    target_link_libraries(${t} PUBLIC Microsoft::DirectXMath)
  endforeach()
endif()

# Model uses dynamic_cast, so we need /GR (Enable RTTI)
if(MSVC)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /Wall /GR /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
      target_link_options(${t} PRIVATE /DYNAMICBASE /NXCOMPAT)
    endforeach()

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_link_options(${t} PRIVATE /SAFESEH)
      endforeach()
    endif()

    if(ENABLE_SPECTRE_MITIGATION
       AND (MSVC_VERSION GREATER_EQUAL 1913)
       AND (NOT WINDOWS_STORE)
       AND (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")))
      message(STATUS "Building Spectre-mitigated libraries.")
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE "/Qspectre")
      endforeach()
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
       AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
        target_link_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      endforeach()
    endif()
else()
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
    endforeach()
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE ${ARCH_SSE2})
    endforeach()
endif()

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsLib "-Wpedantic" "-Wextra")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-gnu-anonymous-struct"
        "-Wno-missing-prototypes" "-Wno-nested-anon-types" "-Wno-unused-const-variable")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
elseif(MINGW)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE -Wno-ignored-attributes)
      target_link_options(${t} PRIVATE -municode)
    endforeach()
elseif( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus /Zc:inline)
    endforeach()

    if(ENABLE_CODE_ANALYSIS)
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /analyze)
      endforeach()
    endif()

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      message(STATUS "Building using Whole Program Optimization")
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /Gy /Gw)
      endforeach()
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /ZH:SHA_256)
        endforeach()
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /Zc:preprocessor /wd5105)
        endforeach()
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_link_options(${t} PRIVATE /CETCOMPAT)
        endforeach()
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /Zc:lambda)
        endforeach()
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /Zc:templateScope /Zc:checkGwOdr)
        endforeach()
    endif()

    set(WarningsEXE "/wd4365" "/wd4514" "/wd4625" "/wd4626" "/wd4627" "/wd4710" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
        list(APPEND WarningsEXE "/wd5262" "/wd5264")
        target_compile_options(${PROJECT_NAME} PRIVATE "/wd5262")
    endif()
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
endif()

if(WIN32)
    if(WINDOWS_STORE)
      target_compile_definitions(${PROJECT_NAME} PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP)
    endif()

    if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10)
        message(STATUS "Using DirectX Tool Kit for Audio on XAudio 2.9 (Windows 10/Windows 11).")
        set(WINVER 0x0A00)
    elseif((${DIRECTX_ARCH} MATCHES "^arm64") OR BUILD_GAMEINPUT OR BUILD_WGI)
        message(STATUS "Building for Windows 10/Windows 11.")
        set(WINVER 0x0A00)
    elseif(BUILD_XAUDIO_WIN8)
        message(STATUS "Using DirectX Tool Kit for Audio on XAudio 2.8 (Windows 8).")
        set(WINVER 0x0602)
    elseif(${DIRECTX_ARCH} MATCHES "^arm")
        message(STATUS "Building for Windows 8.")
        set(WINVER 0x0602)
    else()
        message(STATUS "Building for Windows 7.")
        set(WINVER 0x0601)
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN7_PLATFORM_UPDATE)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})
    endforeach()

    if(DISABLE_MSVC_ITERATOR_DEBUGGING)
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_definitions(${t} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
      endforeach()
    endif()
endif()

if(BUILD_TOOLS AND WIN32)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT xwbtool)
endif()

#--- Test suite
include(CTest)
if(BUILD_TESTING AND WIN32 AND (NOT WINDOWS_STORE)
   AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/CMakeLists.txt"))
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests)
endif()
