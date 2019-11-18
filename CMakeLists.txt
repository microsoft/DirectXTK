# DirectX Tool Kit for DirectX 11
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# http://go.microsoft.com/fwlink/?LinkId=248929

cmake_minimum_required (VERSION 3.11)

project (DirectXTK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

add_library (${PROJECT_NAME} STATIC
    Inc/Audio.h
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
    Inc/WICTextureLoader.h
    Audio/AudioEngine.cpp
    Audio/DynamicSoundEffectInstance.cpp
    Audio/SoundCommon.cpp
    Audio/SoundCommon.h
    Audio/SoundEffect.cpp
    Audio/SoundEffectInstance.cpp
    Audio/WaveBank.cpp
    Audio/WaveBankReader.cpp
    Audio/WaveBankReader.h
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h
    Src/AlignedNew.h
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/Bezier.h
    Src/BinaryReader.cpp
    Src/BinaryReader.h
    Src/CommonStates.cpp
    Src/ConstantBuffer.h
    Src/dds.h
    Src/DDSTextureLoader.cpp
    Src/DebugEffect.cpp
    Src/DemandCreate.h
    Src/DGSLEffect.cpp
    Src/DGSLEffectFactory.cpp
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
    Src/Shaders
    Src/SharedResourcePool.h
    Src/SimpleMath.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/TeapotData.inc
    Src/ToneMapPostProcess.cpp
    Src/vbo.h
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp
)

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC Inc)
target_include_directories(${PROJECT_NAME} PRIVATE Audio Src)

# Use max Warning Level 
string(REPLACE "/W3" "/Wall" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/W3" "/Wall" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/W3" "/Wall" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

# Model uses dynamic_cast, so we need /GR (Enable RTTI)

add_executable(xwbtool
    xwbtool/xwbtool.cpp
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h)
target_include_directories(xwbtool PRIVATE Audio Src)
source_group(xwbtool REGULAR_EXPRESSION XWBTool/*.*)

target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)
target_compile_options(xwbtool PRIVATE /fp:fast)

if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
    target_compile_options(xwbtool PRIVATE /arch:SSE2)
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set(WarningsLib "-Wpedantic" "-Wextra")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-gnu-anonymous-struct"
        "-Wno-missing-prototypes" "-Wno-nested-anon-types" "-Wno-unused-const-variable")
    target_compile_options(xwbtool PRIVATE ${WarningsEXE})
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus)
    target_compile_options(xwbtool PRIVATE /permissive- /Zc:__cplusplus)

    set(WarningsEXE "/wd4365" "/wd4710" "/wd4820" "/wd5039" "/wd5045")
    target_compile_options(xwbtool PRIVATE ${WarningsEXE})
endif()

# Windows 8 is used here to use XAudio 2.8 rather than require the legacy DirectX SDK
add_compile_definitions(_UNICODE UNICODE _WIN32_WINNT=0x0602)
