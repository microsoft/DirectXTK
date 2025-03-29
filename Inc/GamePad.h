//--------------------------------------------------------------------------------------
// File: GamePad.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#if !defined(USING_XINPUT) && !defined(USING_GAMEINPUT) && !defined(USING_WINDOWS_GAMING_INPUT)

#ifdef _GAMING_DESKTOP
#include <grdk.h>
#endif

#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_GAMES)) || (defined(_GAMING_DESKTOP) && (_GRDK_EDITION >= 220600))
#define USING_GAMEINPUT
#elif (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/) && !defined(_GAMING_DESKTOP) && !defined(__MINGW32__)
#define USING_WINDOWS_GAMING_INPUT
#elif !defined(_XBOX_ONE)
#define USING_XINPUT
#endif

#endif // !USING_XINPUT && !USING_GAMEINPUT && !USING_WINDOWS_GAMING_INPUT

#ifdef USING_GAMEINPUT
#include <GameInput.h>
#if !defined(_GAMING_XBOX) && defined(_MSC_VER)
#pragma comment(lib,"gameinput.lib")
#endif

#elif defined(USING_WINDOWS_GAMING_INPUT)
#ifdef _MSC_VER
#pragma comment(lib,"runtimeobject.lib")
#endif
#include <string>

#elif defined(_XBOX_ONE)
// Legacy Xbox One XDK uses Windows::Xbox::Input

#elif defined(USING_XINPUT)
#ifdef _MSC_VER
#pragma comment(lib,"xinput.lib")
#endif
#endif

#include <cstdint>
#include <memory>

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

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif


namespace DirectX
{
    class GamePad
    {
    public:
        DIRECTX_TOOLKIT_API GamePad() noexcept(false);

        DIRECTX_TOOLKIT_API GamePad(GamePad&&) noexcept;
        DIRECTX_TOOLKIT_API GamePad& operator= (GamePad&&) noexcept;

        GamePad(GamePad const&) = delete;
        GamePad& operator=(GamePad const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~GamePad();

    #if defined(USING_GAMEINPUT) || defined(USING_WINDOWS_GAMING_INPUT) || defined(_XBOX_ONE)
        static constexpr int MAX_PLAYER_COUNT = 8;
    #else
        static constexpr int MAX_PLAYER_COUNT = 4;
    #endif

        static constexpr int c_MostRecent = -1;

    #ifdef USING_GAMEINPUT
        static constexpr int c_MergedInput = -2;
    #endif

        enum DeadZone : uint32_t
        {
            DEAD_ZONE_INDEPENDENT_AXES = 0,
            DEAD_ZONE_CIRCULAR,
            DEAD_ZONE_NONE,
        };

        struct Buttons
        {
            bool a;
            bool b;
            bool x;
            bool y;
            bool leftStick;
            bool rightStick;
            bool leftShoulder;
            bool rightShoulder;
            union
            {
                bool back;
                bool view;
            };
            union
            {
                bool start;
                bool menu;
            };
        };

        struct DPad
        {
            bool up;
            bool down;
            bool right;
            bool left;
        };

        struct ThumbSticks
        {
            float leftX;
            float leftY;
            float rightX;
            float rightY;
        };

        struct Triggers
        {
            float left;
            float right;
        };

        struct DIRECTX_TOOLKIT_API State
        {
            bool        connected;
            uint64_t    packet;
            Buttons     buttons;
            DPad        dpad;
            ThumbSticks thumbSticks;
            Triggers    triggers;

            bool __cdecl IsConnected() const noexcept { return connected; }

            // Is the button pressed currently?
            bool __cdecl IsAPressed() const noexcept { return buttons.a; }
            bool __cdecl IsBPressed() const noexcept { return buttons.b; }
            bool __cdecl IsXPressed() const noexcept { return buttons.x; }
            bool __cdecl IsYPressed() const noexcept { return buttons.y; }

            bool __cdecl IsLeftStickPressed() const noexcept { return buttons.leftStick; }
            bool __cdecl IsRightStickPressed() const noexcept { return buttons.rightStick; }

            bool __cdecl IsLeftShoulderPressed() const noexcept { return buttons.leftShoulder; }
            bool __cdecl IsRightShoulderPressed() const noexcept { return buttons.rightShoulder; }

            bool __cdecl IsBackPressed() const noexcept { return buttons.back; }
            bool __cdecl IsViewPressed() const noexcept { return buttons.view; }
            bool __cdecl IsStartPressed() const noexcept { return buttons.start; }
            bool __cdecl IsMenuPressed() const noexcept { return buttons.menu; }

            bool __cdecl IsDPadDownPressed() const noexcept { return dpad.down; }
            bool __cdecl IsDPadUpPressed() const noexcept { return dpad.up; }
            bool __cdecl IsDPadLeftPressed() const noexcept { return dpad.left; }
            bool __cdecl IsDPadRightPressed() const noexcept { return dpad.right; }

            bool __cdecl IsLeftThumbStickUp() const noexcept { return (thumbSticks.leftY > 0.5f) != 0; }
            bool __cdecl IsLeftThumbStickDown() const noexcept { return (thumbSticks.leftY < -0.5f) != 0; }
            bool __cdecl IsLeftThumbStickLeft() const noexcept { return (thumbSticks.leftX < -0.5f) != 0; }
            bool __cdecl IsLeftThumbStickRight() const noexcept { return (thumbSticks.leftX > 0.5f) != 0; }

            bool __cdecl IsRightThumbStickUp() const noexcept { return (thumbSticks.rightY > 0.5f) != 0; }
            bool __cdecl IsRightThumbStickDown() const noexcept { return (thumbSticks.rightY < -0.5f) != 0; }
            bool __cdecl IsRightThumbStickLeft() const noexcept { return (thumbSticks.rightX < -0.5f) != 0; }
            bool __cdecl IsRightThumbStickRight() const noexcept { return (thumbSticks.rightX > 0.5f) != 0; }

            bool __cdecl IsLeftTriggerPressed() const noexcept { return (triggers.left > 0.5f) != 0; }
            bool __cdecl IsRightTriggerPressed() const noexcept { return (triggers.right > 0.5f) != 0; }
        };

        struct DIRECTX_TOOLKIT_API Capabilities
        {
            enum Type : uint32_t
            {
                UNKNOWN = 0,
                GAMEPAD,
                WHEEL,
                ARCADE_STICK,
                FLIGHT_STICK,
                DANCE_PAD,
                GUITAR,
                GUITAR_ALTERNATE,
                DRUM_KIT,
                GUITAR_BASS = 11,
                ARCADE_PAD = 19,
            };

            bool                connected;
            Type                gamepadType;
        #ifdef USING_GAMEINPUT
            APP_LOCAL_DEVICE_ID id;
        #elif defined(USING_WINDOWS_GAMING_INPUT)
            std::wstring        id;
        #else
            uint64_t            id;
        #endif
            uint16_t            vid;
            uint16_t            pid;

            Capabilities() noexcept : connected(false), gamepadType(UNKNOWN), id{}, vid(0), pid(0) {}

            bool __cdecl IsConnected() const noexcept { return connected; }
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

            ButtonState a;
            ButtonState b;
            ButtonState x;
            ButtonState y;

            ButtonState leftStick;
            ButtonState rightStick;

            ButtonState leftShoulder;
            ButtonState rightShoulder;

            union
            {
                ButtonState back;
                ButtonState view;
            };

            union
            {
                ButtonState start;
                ButtonState menu;
            };

            ButtonState dpadUp;
            ButtonState dpadDown;
            ButtonState dpadLeft;
            ButtonState dpadRight;

            ButtonState leftStickUp;
            ButtonState leftStickDown;
            ButtonState leftStickLeft;
            ButtonState leftStickRight;

            ButtonState rightStickUp;
            ButtonState rightStickDown;
            ButtonState rightStickLeft;
            ButtonState rightStickRight;

            ButtonState leftTrigger;
            ButtonState rightTrigger;

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

        // Retrieve the current state of the gamepad of the associated player index
        DIRECTX_TOOLKIT_API State __cdecl GetState(
            int player,
            DeadZone deadZoneMode = DEAD_ZONE_INDEPENDENT_AXES);

        // Retrieve the current capabilities of the gamepad of the associated player index
        DIRECTX_TOOLKIT_API Capabilities __cdecl GetCapabilities(int player);

        // Set the vibration motor speeds of the gamepad
        DIRECTX_TOOLKIT_API bool __cdecl SetVibration(
            int player,
            float leftMotor, float rightMotor,
            float leftTrigger = 0.f, float rightTrigger = 0.f) noexcept;

        // Handle suspending/resuming
        DIRECTX_TOOLKIT_API void __cdecl Suspend() noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Resume() noexcept;

    #ifdef USING_GAMEINPUT
        DIRECTX_TOOLKIT_API void __cdecl RegisterEvents(void* ctrlChanged) noexcept;

        // Underlying device access
        _Success_(return)
        DIRECTX_TOOLKIT_API
        bool __cdecl GetDevice(int player,
        #if defined(GAMEINPUT_API_VERSION) && (GAMEINPUT_API_VERSION == 1)
            _Outptr_ GameInput::v1::IGameInputDevice * *device
        #else
            _Outptr_ IGameInputDevice * *device
        #endif
            ) noexcept;
    #elif defined(USING_WINDOWS_GAMING_INPUT) || defined(_XBOX_ONE)
        DIRECTX_TOOLKIT_API void __cdecl RegisterEvents(void* ctrlChanged, void* userChanged) noexcept;
    #endif

        // Singleton
        DIRECTX_TOOLKIT_API static GamePad& __cdecl Get();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(pop)
#endif
