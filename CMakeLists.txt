# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXTK_VERSION 1.9.1)

if(XBOX_CONSOLE_TARGET STREQUAL "durango")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
endif()

project (DirectXTK
  VERSION ${DIRECTXTK_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 11"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkId=248929"
  LANGUAGES CXX)

if(DEFINED XBOX_CONSOLE_TARGET)
  set(CMAKE_CXX_STANDARD_LIBRARIES "")
endif()

option(BUILD_TOOLS "Build XWBTool" ON)

option(BUILD_SHARED_LIBS "Build DirectXTK as a shared library" OFF)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" OFF)
option(BUILD_XAUDIO_WIN8 "Build for XAudio 2.8" ON)
option(BUILD_XAUDIO_REDIST "Build for XAudio2Redist" OFF)

option(BUILD_GAMEINPUT "Build for GameInput" OFF)
option(BUILD_WGI "Build for Windows.Gaming.Input" OFF)
option(BUILD_XINPUT "Build for XInput" OFF)

# https://devblogs.microsoft.com/cppblog/spectre-mitigations-in-msvc/
option(ENABLE_SPECTRE_MITIGATION "Build using /Qspectre for MSVC" OFF)

option(DISABLE_MSVC_ITERATOR_DEBUGGING "Disable iterator debugging in Debug configurations with the MSVC CRT" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(ENABLE_CODE_COVERAGE "Build with code-coverage" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

option(NO_WCHAR_T "Use legacy wide-character as unsigned short" OFF)

option(BUILD_FUZZING "Build for fuzz testing" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(XBOX_CONSOLE_TARGET STREQUAL "durango")
  set(BUILD_GAMEINPUT OFF)
  set(BUILD_WGI OFF)
  set(BUILD_XINPUT OFF)
  set(BUILD_XBOXONE_SHADERS ON)
  set(BUILD_XAUDIO_WIN10 OFF)
  set(BUILD_XAUDIO_WIN8 ON)
  set(BUILD_TOOLS OFF)
elseif(WINDOWS_STORE)
  set(BUILD_GAMEINPUT OFF)
  set(BUILD_WGI ON)
  set(BUILD_TOOLS OFF)
endif()

include(GNUInstallDirs)
include(build/CompilerAndLinker.cmake)

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
    Src/CMO.h
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

# Xbox-specific extensions
if(DEFINED XBOX_CONSOLE_TARGET)
  set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
      Inc/XboxDDSTextureLoader.h)

  set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
      Src/XboxDDSTextureLoader.cpp)
endif()

# These source files are identical in both DX11 and DX12 version.
set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
    Inc/GamePad.h
    Inc/Keyboard.h
    Inc/Mouse.h
    Inc/SimpleMath.h
    Inc/SimpleMath.inl)

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    Src/BinaryReader.cpp
    Src/GamePad.cpp
    Src/Geometry.cpp
    Src/Keyboard.cpp
    Src/Mouse.cpp
    Src/SimpleMath.cpp)

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
    Src/TeapotData.inc)

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

if(WINDOWS_STORE
   OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8
   OR BUILD_XAUDIO_REDIST)
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

if(BUILD_XBOXONE_SHADERS)
    message(STATUS "Using Shader Model 5.0 for Xbox One for shaders")
    set(ShaderOpts xbox)
else()
    message(STATUS "Using Shader Model 4.0/9.1 for shaders.")
    set(ShaderOpts "")
endif()

if(NOT USE_PREBUILT_SHADERS)
    if(BUILD_XBOXONE_SHADERS)
    else()
        find_program(DIRECTX_FXC_TOOL FXC.EXE
          HINTS "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_SYSTEM_VERSION}/${DIRECTX_HOST_ARCH}"
                "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/${DIRECTX_HOST_ARCH}")
        message(STATUS "Using LegacyShaderCompiler found in ${DIRECTX_FXC_TOOL}")
    endif()
    add_custom_command(
        OUTPUT "${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc"
        MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Generating HLSL shaders..."
        COMMAND ${CMAKE_COMMAND} -E env CompileShadersOutput="${COMPILED_SHADERS}" $<$<BOOL:${DIRECTX_FXC_TOOL}>:LegacyShaderCompiler=${DIRECTX_FXC_TOOL}> CompileShaders.cmd ARGS ${ShaderOpts} > "${COMPILED_SHADERS}/compileshaders.log"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Src/Shaders"
        USES_TERMINAL)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  message(STATUS "Build library as a DLL")

  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/build/DirectXTK.rc.in"
      "${CMAKE_CURRENT_BINARY_DIR}/DirectXTK.rc" @ONLY)

  add_library(${PROJECT_NAME} SHARED ${LIBRARY_SOURCES} ${LIBRARY_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/DirectXTK.rc")

  target_compile_definitions(${PROJECT_NAME} PRIVATE DIRECTX_TOOLKIT_EXPORT)
  target_compile_definitions(${PROJECT_NAME} INTERFACE DIRECTX_TOOLKIT_IMPORT)

  if(XBOX_CONSOLE_TARGET MATCHES "durango")
    target_link_libraries(${PROJECT_NAME} PRIVATE kernelx.lib combase.lib d3d12_x.lib xi.lib)
  endif()

  if(MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE xinput1_4.lib)
  endif()
else()
  add_library(${PROJECT_NAME} ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})
endif()

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

if(WINDOWS_STORE
   OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8
   OR BUILD_XAUDIO_REDIST)
    target_include_directories(${PROJECT_NAME} PRIVATE Audio)
endif()

if(MINGW)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)
else()
    find_package(directxmath CONFIG QUIET)
endif()

if(directxmath_FOUND)
    message(STATUS "Using DirectXMath package")
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXMath)
endif()

if(BUILD_XAUDIO_REDIST
   AND (NOT BUILD_XAUDIO_WIN10) AND (NOT BUILD_XAUDIO_WIN8) AND (NOT WINDOWS_STORE))
    message(STATUS "Using XAudio2Redist for DirectX Tool Kit for Audio.")
    find_package(xaudio2redist CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

include(CheckIncludeFileCXX)

if(BUILD_GAMEINPUT)
    message(STATUS "Using GameInput for GamePad/Keyboard/Mouse.")
    find_package(gameinput CONFIG QUIET)
    if(gameinput_FOUND)
      target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::GameInput)
    else()
      set(CMAKE_REQUIRED_QUIET ON)
      CHECK_INCLUDE_FILE_CXX(GameInput.h GAMEINPUT_HEADER)
      if(NOT GAMEINPUT_HEADER)
          message(FATAL_ERROR "Microsoft GDK required to build GameInput. See https://aka.ms/gdk")
      endif()
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

# Create pkg-config file
include(build/JoinPaths.cmake)
# from: https://github.com/jtojnar/cmake-snips#concatenating-paths-when-building-pkg-config-files
join_paths(DIRECTXTK_INCLUDEDIR_FOR_PKG_CONFIG "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
join_paths(DIRECTXTK_LIBDIR_FOR_PKG_CONFIG "\${prefix}"     "${CMAKE_INSTALL_LIBDIR}")

set(DIRECTXTK_DEP_L "")
if(directxmath_FOUND)
  list(APPEND DIRECTXTK_DEP_L "DirectXMath")
endif()
list(LENGTH DIRECTXTK_DEP_L DEP_L)
if(DEP_L)
  string(REPLACE ";" ", " DIRECTXTK_DEP " ${DIRECTXTK_DEP_L}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build/DirectXTK.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DirectXTK.pc" @ONLY)

# Install the pkg-config file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DirectXTK.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#--- Command-line tools
if(BUILD_TOOLS AND WIN32)
  set(TOOL_EXES xwbtool)

  add_executable(xwbtool
    xwbtool/xwbtool.cpp
    xwbtool/xwbtool.rc
    xwbtool/settings.manifest
    xwbtool/CmdLineHelpers.h
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h)
  target_compile_features(xwbtool PRIVATE cxx_std_17)
  target_include_directories(xwbtool PRIVATE Audio Src)
  target_link_libraries(xwbtool PRIVATE version.lib)
  source_group(xwbtool REGULAR_EXPRESSION XWBTool/*.*)
endif()

if(directxmath_FOUND)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_link_libraries(${t} PUBLIC Microsoft::DirectXMath)
    endforeach()
endif()

if(TOOL_EXES)
  message(STATUS "Building tools: ${TOOL_EXES}")
endif()

# Model uses dynamic_cast, so we need /GR (Enable RTTI)
if(MSVC)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /Wall /EHsc /GR)
    endforeach()

    if(NO_WCHAR_T)
      message(STATUS "Using non-native wchar_t as unsigned short")
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE "/Zc:wchar_t-")
      endforeach()
    endif()
endif()

foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
  target_compile_definitions(${t} PRIVATE ${COMPILER_DEFINES})
  target_compile_options(${t} PRIVATE ${COMPILER_SWITCHES})
  target_link_options(${t} PRIVATE ${LINKER_SWITCHES})
endforeach()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    set(WarningsLib -Wall -Wpedantic -Wextra)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsLib "-Wno-unsafe-buffer-usage")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-switch-default"
        "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-gnu-anonymous-struct"
        "-Wno-missing-prototypes" "-Wno-nested-anon-types" "-Wno-unused-const-variable")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE "-Wno-ignored-attributes" "-Walloc-size-larger-than=4GB")

      if(BUILD_SHARED_LIBS)
        target_compile_options(${t} PRIVATE "-Wno-attributes")
      endif()
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(BUILD_SHARED_LIBS)
      target_compile_options(${PROJECT_NAME} PRIVATE "/wd4251" "/wd4275")
    endif()

    if(ENABLE_CODE_ANALYSIS)
      message(STATUS "Building with Code Analysis (PREFIX)")
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /analyze /WX)
      endforeach()
    endif()

    if(ENABLE_SPECTRE_MITIGATION
       AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.13)
       AND (NOT WINDOWS_STORE))
        message(STATUS "Building Spectre-mitigated libraries")
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE "/Qspectre")
        endforeach()
    endif()

    set(WarningsEXE "/wd4365" "/wd4514" "/wd4571" "/wd4625" "/wd4626" "/wd4627" "/wd4710" "/wd4711" "/wd4774" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE "/wd5262" "/wd5264")
      target_compile_options(${PROJECT_NAME} PRIVATE "/wd5262")
    endif()
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()

    if(BUILD_FUZZING AND (NOT WINDOWS_STORE))
      string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
      string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE ${ASAN_SWITCHES})
          target_link_libraries(${t} PRIVATE ${ASAN_LIBS})
        endforeach()
      endif()
    endif()
endif()

if(WIN32)
    if(XBOX_CONSOLE_TARGET STREQUAL "durango")
        set(WINVER 0x0602)
    elseif(WINDOWS_STORE OR BUILD_XAUDIO_WIN10)
        message(STATUS "Using DirectX Tool Kit for Audio on XAudio 2.9 (Windows 10/Windows 11).")
        set(WINVER 0x0A00)
    elseif((${DIRECTX_ARCH} MATCHES "^arm64") OR BUILD_GAMEINPUT OR BUILD_WGI)
        message(STATUS "Building for Windows 10/Windows 11.")
        set(WINVER 0x0A00)
    elseif(BUILD_XAUDIO_WIN8)
        message(STATUS "Using DirectX Tool Kit for Audio on XAudio 2.8 (Windows 8.1).")
        set(WINVER 0x0603)
    else()
        message(STATUS "Building for Windows 8.1")
        set(WINVER 0x0603)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE _WIN32_WINNT=${WINVER})
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

if(BUILD_TOOLS AND (NOT VCPKG_TOOLCHAIN))
    foreach(t IN LISTS TOOL_EXES)
      install(TARGETS ${t} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endforeach()
endif()

#--- Test suite
if(WIN32 AND (NOT WINDOWS_STORE) AND (NOT (DEFINED XBOX_CONSOLE_TARGET)))
    include(CTest)
    if(BUILD_TESTING AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/CMakeLists.txt"))
        enable_testing()
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests)

        if(ENABLE_CODE_COVERAGE AND (DEFINED COV_COMPILER_SWITCHES))
          target_compile_options(${PROJECT_NAME} PRIVATE ${COV_COMPILER_SWITCHES})
        endif()
    elseif(BUILD_FUZZING AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders/CMakeLists.txt"))
        message(STATUS "Building for fuzzing")
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders)
    endif()
endif()
