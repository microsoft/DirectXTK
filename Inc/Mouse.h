//--------------------------------------------------------------------------------------
// File: Mouse.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#if !defined(USING_XINPUT) && !defined(USING_GAMEINPUT) && !defined(USING_COREWINDOW)

#ifdef _GAMING_DESKTOP
#include <grdk.h>
#endif

#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_GAMES)) || (defined(_GAMING_DESKTOP) && (_GRDK_EDITION >= 220600))
#define USING_GAMEINPUT
#elif (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)) || (defined(_XBOX_ONE) && defined(_TITLE))
#define USING_COREWINDOW
#endif

#endif // !USING_XINPUT && !USING_GAMEINPUT && !USING_WINDOWS_GAMING_INPUT

#if defined(USING_GAMEINPUT) && !defined(_GAMING_XBOX) && defined(_MSC_VER)
#pragma comment(lib,"gameinput.lib")
#endif

#include <cstdint>
#include <memory>

#ifdef USING_COREWINDOW
namespace ABI { namespace Windows { namespace UI { namespace Core { struct ICoreWindow; } } } }
#endif

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


namespace DirectX
{
    class Mouse
    {
    public:
        DIRECTX_TOOLKIT_API Mouse() noexcept(false);

        DIRECTX_TOOLKIT_API Mouse(Mouse&&) noexcept;
        DIRECTX_TOOLKIT_API Mouse& operator= (Mouse&&) noexcept;

        Mouse(Mouse const&) = delete;
        Mouse& operator=(Mouse const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~Mouse();

        enum Mode : uint32_t
        {
            MODE_ABSOLUTE = 0,
            MODE_RELATIVE,
        };

        struct State
        {
            bool    leftButton;
            bool    middleButton;
            bool    rightButton;
            bool    xButton1;
            bool    xButton2;
            int     x;
            int     y;
            int     scrollWheelValue;
            Mode    positionMode;
        };

        class DIRECTX_TOOLKIT_API ButtonStateTracker
        {
        public:
            enum ButtonState : uint32_t
            {
                UP = 0,         // Button is up
                HELD = 1,       // Button is held down
                RELEASED = 2,   // Button was just released
                PRESSED = 3,    // Buton was just pressed
            };

            ButtonState leftButton;
            ButtonState middleButton;
            ButtonState rightButton;
            ButtonState xButton1;
            ButtonState xButton2;

        #ifdef _PREFAST_
        #pragma prefast(push)
        #pragma prefast(disable : 26495, "Reset() performs the initialization")
        #endif
            ButtonStateTracker() noexcept { Reset(); }
        #ifdef _PREFAST_
        #pragma prefast(pop)
        #endif

            void __cdecl Update(const State& state) noexcept;

            void __cdecl Reset() noexcept;

            State __cdecl GetLastState() const noexcept { return lastState; }

        private:
            State lastState;
        };

        // Retrieve the current state of the mouse
        DIRECTX_TOOLKIT_API State __cdecl GetState() const;

        // Resets the accumulated scroll wheel value
        DIRECTX_TOOLKIT_API void __cdecl ResetScrollWheelValue() noexcept;

        // Sets mouse mode (defaults to absolute)
        DIRECTX_TOOLKIT_API void __cdecl SetMode(Mode mode);

        // Signals the end of frame (recommended, but optional)
        DIRECTX_TOOLKIT_API void __cdecl EndOfInputFrame() noexcept;

        // Feature detection
        DIRECTX_TOOLKIT_API bool __cdecl IsConnected() const;

        // Cursor visibility
        DIRECTX_TOOLKIT_API bool __cdecl IsVisible() const noexcept;
        DIRECTX_TOOLKIT_API void __cdecl SetVisible(bool visible);

    #ifdef USING_COREWINDOW
        DIRECTX_TOOLKIT_API void __cdecl SetWindow(ABI::Windows::UI::Core::ICoreWindow* window);
    #ifdef __cplusplus_winrt
        DIRECTX_TOOLKIT_API void __cdecl SetWindow(Windows::UI::Core::CoreWindow^ window)
        {
            // See https://msdn.microsoft.com/en-us/library/hh755802.aspx
            SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
        }
    #endif
    #ifdef CPPWINRT_VERSION
        DIRECTX_TOOLKIT_API void __cdecl SetWindow(winrt::Windows::UI::Core::CoreWindow window)
        {
            // See https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/interop-winrt-abi
            SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(winrt::get_abi(window)));
        }
    #endif

        static void __cdecl SetDpi(float dpi);
    #elif defined(WM_USER)
        DIRECTX_TOOLKIT_API void __cdecl SetWindow(HWND window);
        DIRECTX_TOOLKIT_API static void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

    #ifdef _GAMING_XBOX
        DIRECTX_TOOLKIT_API static void __cdecl SetResolution(float scale);
    #endif
    #endif

        // Singleton
        DIRECTX_TOOLKIT_API static Mouse& __cdecl Get();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
