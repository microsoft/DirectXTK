# This modules provides variables with recommended Compiler and Linker switches
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(COMPILER_DEFINES "")
set(COMPILER_SWITCHES "")
set(LINKER_SWITCHES "")

#--- Determines target architecture if not explicitly set
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
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

#--- Build with Unicode Win32 APIs per "UTF-8 Everywhere"
if(WIN32)
  list(APPEND COMPILER_DEFINES _UNICODE UNICODE)
endif()

#--- General MSVC-like SDL options
if(MSVC)
    list(APPEND COMPILER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    list(APPEND LINKER_SWITCHES /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

    if((${DIRECTX_ARCH} MATCHES "x86")
       OR ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm"))))
      list(APPEND LINKER_SWITCHES /SAFESEH)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
       AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND (NOT (TARGET OpenEXR::OpenEXR))
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
          list(APPEND COMPILER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
          list(APPEND LINKER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()
else()
    list(APPEND COMPILER_DEFINES $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

#--- Target architecture switches
if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
    list(APPEND COMPILER_SWITCHES $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX2,-march=znver2>)
elseif(XBOX_CONSOLE_TARGET STREQUAL "xboxone|durago")
    list(APPEND COMPILER_SWITCHES  $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX,-march=btver2>)
elseif(NOT (${DIRECTX_ARCH} MATCHES "^arm"))
    if((${DIRECTX_ARCH} MATCHES "x86") OR (CMAKE_SIZEOF_VOID_P EQUAL 4))
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC,Intel>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC,Intel>>:-msse2>)
    endif()

    list(APPEND COMPILER_SWITCHES ${ARCH_SSE2})
endif()

#--- Compiler-specific switches
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    if(MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0))
      list(APPEND COMPILER_SWITCHES /ZH:SHA_256)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    list(APPEND COMPILER_SWITCHES /Zc:__cplusplus /Zc:inline /fp:fast /Qdiag-disable:161)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    list(APPEND COMPILER_SWITCHES /sdl /permissive- /JMC- /Zc:__cplusplus /Zc:inline /fp:fast)

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      message(STATUS "Building using Whole Program Optimization")
      list(APPEND COMPILER_SWITCHES /Gy /Gw)
    endif()

    if(OpenMP_CXX_FOUND)
      # OpenMP in MSVC is not compatible with /permissive- unless you disable two-phase lookup
      list(APPEND COMPILER_SWITCHES /Zc:twoPhase-)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
      list(APPEND COMPILER_SWITCHES /ZH:SHA_256)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
       AND (NOT (XBOX_CONSOLE_TARGET STREQUAL "durango")))
        list(APPEND COMPILER_SWITCHES /Zc:preprocessor /wd5104 /wd5105)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
      list(APPEND LINKER_SWITCHES /CETCOMPAT)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
      list(APPEND COMPILER_SWITCHES /Zc:lambda)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.31)
       AND (XBOX_CONSOLE_TARGET STREQUAL "durango"))
        list(APPEND COMPILER_SWITCHES /Zc:static_assert-)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
      list(APPEND COMPILER_SWITCHES /Zc:checkGwOdr)

      if(NOT (DEFINED XBOX_CONSOLE_TARGET))
        list(APPEND COMPILER_SWITCHES $<$<VERSION_GREATER_EQUAL:${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION},10.0.22000>:/Zc:templateScope>)
      endif()
    endif()
endif()

#--- Windows API Family
include(CheckIncludeFileCXX)

if(DEFINED XBOX_CONSOLE_TARGET)
    message(STATUS "Building for Xbox Console Target: ${XBOX_CONSOLE_TARGET}")
    set(CMAKE_REQUIRED_QUIET ON)
    if(XBOX_CONSOLE_TARGET STREQUAL "durango")
        CHECK_INCLUDE_FILE_CXX(xdk.h XDKLegacy_HEADER)
        if(NOT XDKLegacy_HEADER)
            message(FATAL_ERROR "Legacy Xbox One XDK required to build for Durango.")
        endif()
        list(APPEND COMPILER_DEFINES WINAPI_FAMILY=WINAPI_FAMILY_TV_TITLE _XBOX_ONE _TITLE MONOLITHIC=1)
    else()
        CHECK_INCLUDE_FILE_CXX(gxdk.h GXDK_HEADER)
        if(NOT GXDK_HEADER)
            message(FATAL_ERROR "Microsoft GDK with Xbox Extensions required to build for Xbox. See https://aka.ms/gdkx")
        endif()
        list(APPEND COMPILER_DEFINES WINAPI_FAMILY=WINAPI_FAMILY_GAMES)
        if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
            CHECK_INCLUDE_FILE_CXX(d3d12_xs.h D3D12XS_HEADER)
            if(NOT D3D12XS_HEADER)
                message(FATAL_ERROR "Microsoft GDK with Xbox Extensions environment needs to be set for Xbox Series X|S.")
            endif()
            list(APPEND COMPILER_DEFINES _GAMING_XBOX _GAMING_XBOX_SCARLETT)
        elseif(XBOX_CONSOLE_TARGET STREQUAL "xboxone")
            CHECK_INCLUDE_FILE_CXX(d3d12_x.h D3D12X_HEADER)
            if(NOT D3D12X_HEADER)
                message(FATAL_ERROR "Microsoft GDK with Xbox Extensions environment needs to be set for Xbox One.")
            endif()
            list(APPEND COMPILER_DEFINES _GAMING_XBOX _GAMING_XBOX_XBOXONE)
        else()
            message(FATAL_ERROR "Unknown XBOX_CONSOLE_TARGET")
        endif()
    endif()
elseif(WINDOWS_STORE)
    list(APPEND COMPILER_DEFINES WINAPI_FAMILY=WINAPI_FAMILY_APP)
endif()

#--- Address Sanitizer switches
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(ASAN_SWITCHES /fsanitize=address /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
    set(ASAN_LIBS sancov.lib)
endif()
