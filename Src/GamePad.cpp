//--------------------------------------------------------------------------------------
// File: GamePad.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "GamePad.h"
#include "PlatformHelpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


namespace
{
    const float c_XboxOneThumbDeadZone = .24f;  // Recommended Xbox One controller deadzone

    float ApplyLinearDeadZone(float value, float maxValue, float deadZoneSize)
    {
        if (value < -deadZoneSize)
        {
            // Increase negative values to remove the deadzone discontinuity.
            value += deadZoneSize;
        }
        else if (value > deadZoneSize)
        {
            // Decrease positive values to remove the deadzone discontinuity.
            value -= deadZoneSize;
        }
        else
        {
            // Values inside the deadzone come out zero.
            return 0;
        }

        // Scale into 0-1 range.
        float scaledValue = value / (maxValue - deadZoneSize);
        return std::max(-1.f, std::min(scaledValue, 1.f));
    }

    void ApplyStickDeadZone(float x, float y, GamePad::DeadZone deadZoneMode, float maxValue, float deadZoneSize,
                            _Out_ float& resultX, _Out_ float& resultY)
    {
        switch (deadZoneMode)
        {
            case GamePad::DEAD_ZONE_INDEPENDENT_AXES:
                resultX = ApplyLinearDeadZone(x, maxValue, deadZoneSize);
                resultY = ApplyLinearDeadZone(y, maxValue, deadZoneSize);
                break;

            case GamePad::DEAD_ZONE_CIRCULAR:
            {
                float dist = sqrtf(x*x + y * y);
                float wanted = ApplyLinearDeadZone(dist, maxValue, deadZoneSize);

                float scale = (wanted > 0.f) ? (wanted / dist) : 0.f;

                resultX = std::max(-1.f, std::min(x * scale, 1.f));
                resultY = std::max(-1.f, std::min(y * scale, 1.f));
            }
            break;

            default: // GamePad::DEAD_ZONE_NONE
                resultX = ApplyLinearDeadZone(x, maxValue, 0);
                resultY = ApplyLinearDeadZone(y, maxValue, 0);
                break;
        }
    }
}


#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

//======================================================================================
// Windows::Gaming::Input (Windows 10)
//======================================================================================

#pragma warning(push)
#pragma warning(disable : 4471)
#include <windows.gaming.input.h>
#pragma warning(pop)

class GamePad::Impl
{
public:
    Impl(GamePad* owner) :
        mOwner(owner),
        mCtrlChanged(INVALID_HANDLE_VALUE),
        mUserChanged(INVALID_HANDLE_VALUE),
        mMostRecentGamepad(0),
        mStatics{},
        mGamePad{},
        mUserChangeToken{},
        mAddedToken{},
        mRemovedToken{},
        mChanged{}
    {
        using namespace Microsoft::WRL;
        using namespace Microsoft::WRL::Wrappers;
        using namespace ABI::Windows::Foundation;

        if (s_gamePad)
        {
            throw std::exception("GamePad is a singleton");
        }

        s_gamePad = this;

        mChanged.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mChanged)
        {
            throw std::exception("CreateEventEx");
        }

        ThrowIfFailed(GetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_Gamepad).Get(), mStatics.GetAddressOf()));

        typedef __FIEventHandler_1_Windows__CGaming__CInput__CGamepad AddedHandler;
        ThrowIfFailed(mStatics->add_GamepadAdded(Callback<AddedHandler>(GamepadAdded).Get(), &mAddedToken));

        typedef __FIEventHandler_1_Windows__CGaming__CInput__CGamepad RemovedHandler;
        ThrowIfFailed(mStatics->add_GamepadRemoved(Callback<RemovedHandler>(GamepadRemoved).Get(), &mRemovedToken));

        ScanGamePads();
    }

    ~Impl()
    {
        using namespace ABI::Windows::Gaming::Input;

        for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
        {
            if (mGamePad[j])
            {
                ComPtr<IGameController> ctrl;
                HRESULT hr = mGamePad[j].As(&ctrl);
                if (SUCCEEDED(hr) && ctrl)
                {
                    (void)ctrl->remove_UserChanged(mUserChangeToken[j]);
                    mUserChangeToken[j].value = 0;
                }

                mGamePad[j].Reset();
            }
        }

        if (mStatics)
        {
            (void)mStatics->remove_GamepadAdded(mAddedToken);
            mAddedToken.value = 0;

            (void)mStatics->remove_GamepadRemoved(mRemovedToken);
            mRemovedToken.value = 0;

            mStatics.Reset();
        }

        s_gamePad = nullptr;
    }

    void GetState(int player, _Out_ State& state, DeadZone deadZoneMode)
    {
        using namespace Microsoft::WRL;
        using namespace ABI::Windows::Gaming::Input;

        if (WaitForSingleObjectEx(mChanged.get(), 0, FALSE) == WAIT_OBJECT_0)
        {
            ScanGamePads();
        }

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                GamepadReading reading;
                HRESULT hr = mGamePad[player]->GetCurrentReading(&reading);
                if (SUCCEEDED(hr))
                {
                    state.connected = true;
                    state.packet = reading.Timestamp;

                    state.buttons.a = (reading.Buttons & GamepadButtons::GamepadButtons_A) != 0;
                    state.buttons.b = (reading.Buttons & GamepadButtons::GamepadButtons_B) != 0;
                    state.buttons.x = (reading.Buttons & GamepadButtons::GamepadButtons_X) != 0;
                    state.buttons.y = (reading.Buttons & GamepadButtons::GamepadButtons_Y) != 0;

                    state.buttons.leftStick = (reading.Buttons & GamepadButtons::GamepadButtons_LeftThumbstick) != 0;
                    state.buttons.rightStick = (reading.Buttons & GamepadButtons::GamepadButtons_RightThumbstick) != 0;

                    state.buttons.leftShoulder = (reading.Buttons & GamepadButtons::GamepadButtons_LeftShoulder) != 0;
                    state.buttons.rightShoulder = (reading.Buttons & GamepadButtons::GamepadButtons_RightShoulder) != 0;

                    state.buttons.back = (reading.Buttons & GamepadButtons::GamepadButtons_View) != 0;
                    state.buttons.start = (reading.Buttons & GamepadButtons::GamepadButtons_Menu) != 0;

                    state.dpad.up = (reading.Buttons & GamepadButtons::GamepadButtons_DPadUp) != 0;
                    state.dpad.down = (reading.Buttons & GamepadButtons::GamepadButtons_DPadDown) != 0;
                    state.dpad.right = (reading.Buttons & GamepadButtons::GamepadButtons_DPadRight) != 0;
                    state.dpad.left = (reading.Buttons & GamepadButtons::GamepadButtons_DPadLeft) != 0;

                    ApplyStickDeadZone(static_cast<float>(reading.LeftThumbstickX), static_cast<float>(reading.LeftThumbstickY),
                                       deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                                       state.thumbSticks.leftX, state.thumbSticks.leftY);

                    ApplyStickDeadZone(static_cast<float>(reading.RightThumbstickX), static_cast<float>(reading.RightThumbstickY),
                                       deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                                       state.thumbSticks.rightX, state.thumbSticks.rightY);

                    state.triggers.left = static_cast<float>(reading.LeftTrigger);
                    state.triggers.right = static_cast<float>(reading.RightTrigger);

                    return;
                }
            }
        }

        memset(&state, 0, sizeof(State));
    }

    void GetCapabilities(int player, Capabilities& caps)
    {
        using namespace Microsoft::WRL;
        using namespace Microsoft::WRL::Wrappers;
        using namespace ABI::Windows::Foundation;
        using namespace ABI::Windows::System;
        using namespace ABI::Windows::Gaming::Input;

        if (WaitForSingleObjectEx(mChanged.get(), 0, FALSE) == WAIT_OBJECT_0)
        {
            ScanGamePads();
        }

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                caps.connected = true;
                caps.gamepadType = Capabilities::GAMEPAD;
                caps.id.clear();
                caps.vid = caps.pid = 0;

                ComPtr<IGameController> ctrl;
                HRESULT hr = mGamePad[player].As(&ctrl);
                if (SUCCEEDED(hr) && ctrl)
                {
                    ComPtr<IUser> user;
                    hr = ctrl->get_User(user.GetAddressOf());
                    if (SUCCEEDED(hr) && user != nullptr)
                    {
                        HString str;
                        hr = user->get_NonRoamableId(str.GetAddressOf());
                        if (SUCCEEDED(hr))
                        {
                            caps.id = str.GetRawBuffer(nullptr);
                        }
                    }

                // Requires the Windows 10 Creators Update SDK (15063)
                #if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
                    ComPtr<IRawGameControllerStatics> rawStatics;
                    hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_RawGameController).Get(), rawStatics.GetAddressOf());
                    if (SUCCEEDED(hr))
                    {
                        ComPtr<IRawGameController> raw;
                        hr = rawStatics->FromGameController(ctrl.Get(), raw.GetAddressOf());
                        if (SUCCEEDED(hr) && raw)
                        {
                            if (FAILED(raw->get_HardwareVendorId(&caps.vid)))
                                caps.vid = 0;

                            if (FAILED(raw->get_HardwareProductId(&caps.pid)))
                                caps.pid = 0;
                        }
                    }
                #endif // NTDDI_WIN10_RS2
                }
                return;
            }
        }

        caps.id.clear();
        caps = {};
    }

    bool SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger)
    {
        using namespace ABI::Windows::Gaming::Input;

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                GamepadVibration vib;
                vib.LeftMotor = double(leftMotor);
                vib.RightMotor = double(rightMotor);
                vib.LeftTrigger = double(leftTrigger);
                vib.RightTrigger = double(rightTrigger);
                HRESULT hr = mGamePad[player]->put_Vibration(vib);

                if (SUCCEEDED(hr))
                    return true;
            }
        }

        return false;
    }

    void Suspend()
    {
        for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
        {
            mGamePad[j].Reset();
        }
    }

    void Resume()
    {
        // Make sure we rescan gamepads
        SetEvent(mChanged.get());
    }

    GamePad*    mOwner;

    static GamePad::Impl* s_gamePad;

    HANDLE mCtrlChanged;
    HANDLE mUserChanged;

private:
    int mMostRecentGamepad;

    void ScanGamePads()
    {
        using namespace Microsoft::WRL;
        using namespace ABI::Windows::Foundation::Collections;
        using namespace ABI::Windows::Gaming::Input;

        ComPtr<IVectorView<Gamepad*>> pads;
        ThrowIfFailed(mStatics->get_Gamepads(pads.GetAddressOf()));

        unsigned int count = 0;
        ThrowIfFailed(pads->get_Size(&count));

        // Check for removed gamepads
        for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
        {
            if (mGamePad[j])
            {
                unsigned int k = 0;
                for (; k < count; ++k)
                {
                    ComPtr<IGamepad> pad;
                    HRESULT hr = pads->GetAt(k, pad.GetAddressOf());
                    if (SUCCEEDED(hr) && (pad == mGamePad[j]))
                    {
                        break;
                    }
                }

                if (k >= count)
                {
                    ComPtr<IGameController> ctrl;
                    HRESULT hr = mGamePad[j].As(&ctrl);
                    if (SUCCEEDED(hr) && ctrl)
                    {
                        (void)ctrl->remove_UserChanged(mUserChangeToken[j]);
                        mUserChangeToken[j].value = 0;
                    }

                    mGamePad[j].Reset();
                }
            }
        }

        // Check for added gamepads
        for (unsigned int j = 0; j < count; ++j)
        {
            ComPtr<IGamepad> pad;
            HRESULT hr = pads->GetAt(j, pad.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                size_t empty = MAX_PLAYER_COUNT;
                size_t k = 0;
                for (; k < MAX_PLAYER_COUNT; ++k)
                {
                    if (mGamePad[k] == pad)
                    {
                        if (j == (count - 1))
                            mMostRecentGamepad = static_cast<int>(k);
                        break;
                    }
                    else if (!mGamePad[k])
                    {
                        if (empty >= MAX_PLAYER_COUNT)
                            empty = k;
                    }
                }

                if (k >= MAX_PLAYER_COUNT)
                {
                    // Silently ignore "extra" gamepads as there's no hard limit
                    if (empty < MAX_PLAYER_COUNT)
                    {
                        mGamePad[empty] = pad;
                        if (j == (count - 1))
                            mMostRecentGamepad = static_cast<int>(empty);

                        ComPtr<IGameController> ctrl;
                        hr = pad.As(&ctrl);
                        if (SUCCEEDED(hr) && ctrl)
                        {
                            typedef __FITypedEventHandler_2_Windows__CGaming__CInput__CIGameController_Windows__CSystem__CUserChangedEventArgs UserHandler;
                            ThrowIfFailed(ctrl->add_UserChanged(Callback<UserHandler>(UserChanged).Get(), &mUserChangeToken[empty]));
                        }
                    }
                }
            }
        }
    }

    ComPtr<ABI::Windows::Gaming::Input::IGamepadStatics> mStatics;
    ComPtr<ABI::Windows::Gaming::Input::IGamepad> mGamePad[MAX_PLAYER_COUNT];
    EventRegistrationToken mUserChangeToken[MAX_PLAYER_COUNT];

    EventRegistrationToken mAddedToken;
    EventRegistrationToken mRemovedToken;

    ScopedHandle mChanged;

    static HRESULT GamepadAdded(IInspectable *, ABI::Windows::Gaming::Input::IGamepad*)
    {
        if (s_gamePad)
        {
            SetEvent(s_gamePad->mChanged.get());

            if (s_gamePad->mCtrlChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(s_gamePad->mCtrlChanged);
            }
        }
        return S_OK;
    }

    static HRESULT GamepadRemoved(IInspectable *, ABI::Windows::Gaming::Input::IGamepad*)
    {
        if (s_gamePad)
        {
            SetEvent(s_gamePad->mChanged.get());

            if (s_gamePad->mCtrlChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(s_gamePad->mCtrlChanged);
            }
        }
        return S_OK;
    }

    static HRESULT UserChanged(ABI::Windows::Gaming::Input::IGameController*, ABI::Windows::System::IUserChangedEventArgs*)
    {
        if (s_gamePad)
        {
            if (s_gamePad->mUserChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(s_gamePad->mUserChanged);
            }
        }
        return S_OK;
    }
};

GamePad::Impl* GamePad::Impl::s_gamePad = nullptr;


#elif defined(_XBOX_ONE)

//======================================================================================
// Windows::Xbox::Input (Xbox One)
//======================================================================================

#include <Windows.Xbox.Input.h>

#include <Windows.Foundation.Collections.h>

class GamePad::Impl
{
public:
    class GamepadAddedListener : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        ABI::Windows::Foundation::IEventHandler<ABI::Windows::Xbox::Input::GamepadAddedEventArgs *>,
        Microsoft::WRL::FtmBase>
    {
    public:
        GamepadAddedListener(HANDLE event) : mEvent(event) {}

        STDMETHOD(Invoke)(_In_ IInspectable *, _In_ ABI::Windows::Xbox::Input::IGamepadAddedEventArgs *) override
        {
            SetEvent(mEvent);

            auto pad = GamePad::Impl::s_gamePad;

            if (pad && pad->mCtrlChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(pad->mCtrlChanged);
            }
            return S_OK;
        }

    private:
        HANDLE mEvent;
    };

    class GamepadRemovedListener : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        ABI::Windows::Foundation::IEventHandler<ABI::Windows::Xbox::Input::GamepadRemovedEventArgs *>,
        Microsoft::WRL::FtmBase>
    {
    public:
        GamepadRemovedListener(HANDLE event) : mEvent(event) {}

        STDMETHOD(Invoke)(_In_ IInspectable *, _In_ ABI::Windows::Xbox::Input::IGamepadRemovedEventArgs *) override
        {
            SetEvent(mEvent);

            auto pad = GamePad::Impl::s_gamePad;

            if (pad && pad->mCtrlChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(pad->mCtrlChanged);
            }
            return S_OK;
        }

    private:
        HANDLE mEvent;
    };

    class UserPairingListener : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        ABI::Windows::Foundation::IEventHandler<ABI::Windows::Xbox::Input::ControllerPairingChangedEventArgs *>,
        Microsoft::WRL::FtmBase>
    {
    public:
        UserPairingListener() noexcept {}

        STDMETHOD(Invoke)(_In_ IInspectable *, _In_ ABI::Windows::Xbox::Input::IControllerPairingChangedEventArgs *) override
        {
            auto pad = GamePad::Impl::s_gamePad;

            if (pad && pad->mUserChanged != INVALID_HANDLE_VALUE)
            {
                SetEvent(pad->mUserChanged);
            }
            return S_OK;
        }
    };

    Impl(GamePad *owner) :
        mOwner(owner),
        mCtrlChanged(INVALID_HANDLE_VALUE),
        mUserChanged(INVALID_HANDLE_VALUE),
        mMostRecentGamepad(0),
        mStatics{},
        mStaticsCtrl{},
        mGamePad{},
        mAddedToken{},
        mRemovedToken{},
        mUserParingToken{},
        mChanged{}
    {
        using namespace Microsoft::WRL;
        using namespace Microsoft::WRL::Wrappers;
        using namespace ABI::Windows::Foundation;

        if (s_gamePad)
        {
            throw std::exception("GamePad is a singleton");
        }

        s_gamePad = this;

        mChanged.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!mChanged)
        {
            throw std::exception("CreateEventEx");
        }

        ThrowIfFailed(GetActivationFactory(HStringReference(RuntimeClass_Windows_Xbox_Input_Gamepad).Get(), mStatics.GetAddressOf()));

        ThrowIfFailed(GetActivationFactory(HStringReference(RuntimeClass_Windows_Xbox_Input_Controller).Get(), mStaticsCtrl.GetAddressOf()));

        ThrowIfFailed(mStatics->add_GamepadAdded(Make<GamepadAddedListener>(mChanged.get()).Get(), &mAddedToken));

        ThrowIfFailed(mStatics->add_GamepadRemoved(Make<GamepadRemovedListener>(mChanged.get()).Get(), &mRemovedToken));

        ThrowIfFailed(mStaticsCtrl->add_ControllerPairingChanged(Make<UserPairingListener>().Get(), &mUserParingToken));

        ScanGamePads();
    }

    ~Impl()
    {
        if (mStatics)
        {
            (void)mStatics->remove_GamepadAdded(mAddedToken);
            mAddedToken.value = 0;

            (void)mStatics->remove_GamepadRemoved(mRemovedToken);
            mRemovedToken.value = 0;

            mStatics.Reset();
        }

        if (mStaticsCtrl)
        {
            (void)mStaticsCtrl->remove_ControllerPairingChanged(mUserParingToken);
            mUserParingToken.value = 0;

            mStaticsCtrl.Reset();
        }

        s_gamePad = nullptr;
    }

    void GetState(int player, _Out_ State& state, DeadZone deadZoneMode)
    {
        using namespace Microsoft::WRL;
        using namespace ABI::Windows::Xbox::Input;

        if (WaitForSingleObjectEx(mChanged.get(), 0, FALSE) == WAIT_OBJECT_0)
        {
            ScanGamePads();
        }

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                RawGamepadReading reading;
                HRESULT hr = mGamePad[player]->GetRawCurrentReading(&reading);
                if (SUCCEEDED(hr))
                {
                    state.connected = true;
                    state.packet = reading.Timestamp;

                    state.buttons.a = (reading.Buttons & GamepadButtons::GamepadButtons_A) != 0;
                    state.buttons.b = (reading.Buttons & GamepadButtons::GamepadButtons_B) != 0;
                    state.buttons.x = (reading.Buttons & GamepadButtons::GamepadButtons_X) != 0;
                    state.buttons.y = (reading.Buttons & GamepadButtons::GamepadButtons_Y) != 0;

                    state.buttons.leftStick = (reading.Buttons & GamepadButtons::GamepadButtons_LeftThumbstick) != 0;
                    state.buttons.rightStick = (reading.Buttons & GamepadButtons::GamepadButtons_RightThumbstick) != 0;

                    state.buttons.leftShoulder = (reading.Buttons & GamepadButtons::GamepadButtons_LeftShoulder) != 0;
                    state.buttons.rightShoulder = (reading.Buttons & GamepadButtons::GamepadButtons_RightShoulder) != 0;

                    state.buttons.back = (reading.Buttons & GamepadButtons::GamepadButtons_View) != 0;
                    state.buttons.start = (reading.Buttons & GamepadButtons::GamepadButtons_Menu) != 0;

                    state.dpad.up = (reading.Buttons & GamepadButtons::GamepadButtons_DPadUp) != 0;
                    state.dpad.down = (reading.Buttons & GamepadButtons::GamepadButtons_DPadDown) != 0;
                    state.dpad.right = (reading.Buttons & GamepadButtons::GamepadButtons_DPadRight) != 0;
                    state.dpad.left = (reading.Buttons & GamepadButtons::GamepadButtons_DPadLeft) != 0;

                    ApplyStickDeadZone(reading.LeftThumbstickX, reading.LeftThumbstickY,
                                       deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                                       state.thumbSticks.leftX, state.thumbSticks.leftY);

                    ApplyStickDeadZone(reading.RightThumbstickX, reading.RightThumbstickY,
                                       deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                                       state.thumbSticks.rightX, state.thumbSticks.rightY);

                    state.triggers.left = reading.LeftTrigger;
                    state.triggers.right = reading.RightTrigger;

                    return;
                }
            }
        }

        memset(&state, 0, sizeof(State));
    }

    void GetCapabilities(int player, _Out_ Capabilities& caps)
    {
        using namespace Microsoft::WRL;
        using namespace ABI::Windows::Xbox::Input;

        if (WaitForSingleObjectEx(mChanged.get(), 0, FALSE) == WAIT_OBJECT_0)
        {
            ScanGamePads();
        }

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                caps.connected = true;
                caps.gamepadType = Capabilities::UNKNOWN;
                caps.id = 0;
                caps.vid = caps.pid = 0;

                ComPtr<IController> ctrl;
                HRESULT hr = mGamePad[player].As(&ctrl);
                if (SUCCEEDED(hr) && ctrl)
                {
                    hr = ctrl->get_Id(&caps.id);
                    if (FAILED(hr))
                        caps.id = 0;

                    Wrappers::HString str;
                    hr = ctrl->get_Type(str.GetAddressOf());
                    if (SUCCEEDED(hr))
                    {
                        const wchar_t* typeStr = str.GetRawBuffer(nullptr);
                        if (_wcsicmp(typeStr, L"Windows.Xbox.Input.Gamepad") == 0)
                        {
                            caps.gamepadType = Capabilities::GAMEPAD;
                        }
                        else if (_wcsicmp(typeStr, L"Microsoft.Xbox.Input.ArcadeStick") == 0)
                        {
                            caps.gamepadType = Capabilities::ARCADE_STICK;
                        }
                        else if (_wcsicmp(typeStr, L"Microsoft.Xbox.Input.Wheel") == 0)
                        {
                            caps.gamepadType = Capabilities::WHEEL;
                        }
                    }
                }

            #if _XDK_VER >= 0x42ED07E4 /* XDK Edition 180400 */
                ComPtr<IController3> ctrl3;
                hr = mGamePad[player].As(&ctrl3);
                if (SUCCEEDED(hr) && ctrl3)
                {
                    if (FAILED(ctrl3->get_HardwareVendorId(&caps.vid)))
                        caps.vid = 0;

                    if (FAILED(ctrl3->get_HardwareProductId(&caps.pid)))
                        caps.pid = 0;
                }
            #endif

                return;
            }
        }

        memset(&caps, 0, sizeof(Capabilities));
    }

    bool SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger)
    {
        using namespace ABI::Windows::Xbox::Input;

        if (player == -1)
            player = mMostRecentGamepad;

        if ((player >= 0) && (player < MAX_PLAYER_COUNT))
        {
            if (mGamePad[player])
            {
                HRESULT hr;
                try
                {
                    GamepadVibration vib;
                    vib.LeftMotorLevel = leftMotor;
                    vib.RightMotorLevel = rightMotor;
                    vib.LeftTriggerLevel = leftTrigger;
                    vib.RightTriggerLevel = rightTrigger;
                    hr = mGamePad[player]->SetVibration(vib);
                }
                catch (...)
                {
                    // Handle case where gamepad might be invalid
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr))
                    return true;
            }
        }

        return false;
    }

    void Suspend()
    {
        for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
        {
            mGamePad[j].Reset();
        }
    }

    void Resume()
    {
        // Make sure we rescan gamepads
        SetEvent(mChanged.get());
    }

    GamePad*    mOwner;

    static GamePad::Impl* s_gamePad;

    HANDLE mCtrlChanged;
    HANDLE mUserChanged;

private:
    int mMostRecentGamepad;

    void ScanGamePads()
    {
        using namespace ABI::Windows::Foundation::Collections;
        using namespace ABI::Windows::Xbox::Input;

        ComPtr<IVectorView<IGamepad*>> pads;
        ThrowIfFailed(mStatics->get_Gamepads(pads.GetAddressOf()));

        unsigned int count = 0;
        ThrowIfFailed(pads->get_Size(&count));

        // Check for removed gamepads
        for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
        {
            if (mGamePad[j])
            {
                unsigned int k = 0;
                for (; k < count; ++k)
                {
                    ComPtr<IGamepad> pad;
                    HRESULT hr = pads->GetAt(k, pad.GetAddressOf());
                    if (SUCCEEDED(hr) && (pad == mGamePad[j]))
                    {
                        break;
                    }
                }

                if (k >= count)
                {
                    mGamePad[j].Reset();
                }
            }
        }

        // Check for added gamepads
        for (unsigned int j = 0; j < count; ++j)
        {
            ComPtr<IGamepad> pad;
            HRESULT hr = pads->GetAt(j, pad.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                size_t empty = MAX_PLAYER_COUNT;
                size_t k = 0;
                for (; k < MAX_PLAYER_COUNT; ++k)
                {
                    if (mGamePad[k] == pad)
                    {
                        if (!j)
                            mMostRecentGamepad = static_cast<int>(k);
                        break;
                    }
                    else if (!mGamePad[k])
                    {
                        if (empty >= MAX_PLAYER_COUNT)
                            empty = k;
                    }
                }

                if (k >= MAX_PLAYER_COUNT)
                {
                    if (empty >= MAX_PLAYER_COUNT)
                    {
                        throw std::exception("Too many gamepads found");
                    }

                    mGamePad[empty] = pad;
                    if (!j)
                        mMostRecentGamepad = static_cast<int>(empty);
                }
            }
        }
    }

    ComPtr<ABI::Windows::Xbox::Input::IGamepadStatics> mStatics;
    ComPtr<ABI::Windows::Xbox::Input::IControllerStatics> mStaticsCtrl;
    ComPtr<ABI::Windows::Xbox::Input::IGamepad> mGamePad[MAX_PLAYER_COUNT];

    EventRegistrationToken mAddedToken;
    EventRegistrationToken mRemovedToken;
    EventRegistrationToken mUserParingToken;

    ScopedHandle mChanged;
};

GamePad::Impl* GamePad::Impl::s_gamePad = nullptr;

#else

//======================================================================================
// XInput
//======================================================================================

#include <Xinput.h>

static_assert(GamePad::MAX_PLAYER_COUNT == XUSER_MAX_COUNT, "xinput.h mismatch");

class GamePad::Impl
{
public:
    Impl(GamePad* owner) :
        mOwner(owner),
        mConnected{},
        mLastReadTime{}
    #if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
        , mLeftMotor{}
        , mRightMotor{}
        , mSuspended(false)
    #endif
    {
        for (int j = 0; j < XUSER_MAX_COUNT; ++j)
        {
            ClearSlot(j, 0);
        }

        if (s_gamePad)
        {
            throw std::exception("GamePad is a singleton");
        }

        s_gamePad = this;
    }

    ~Impl()
    {
        s_gamePad = nullptr;
    }

    void GetState(int player, _Out_ State& state, DeadZone deadZoneMode)
    {
        if (player == -1)
            player = GetMostRecent();

        ULONGLONG time = GetTickCount64();

        if (!ThrottleRetry(player, time))
        {
        #if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
            if (mSuspended)
            {
                memset(&state, 0, sizeof(State));
                state.connected = mConnected[player];
                return;
            }
        #endif

            XINPUT_STATE xstate;
            DWORD result = XInputGetState(DWORD(player), &xstate);
            if (result == ERROR_DEVICE_NOT_CONNECTED)
            {
                ClearSlot(player, time);
            }
            else
            {
                if (!mConnected[player])
                    mLastReadTime[player] = time;

                mConnected[player] = true;

                state.connected = true;
                state.packet = xstate.dwPacketNumber;

                WORD xbuttons = xstate.Gamepad.wButtons;
                state.buttons.a = (xbuttons & XINPUT_GAMEPAD_A) != 0;
                state.buttons.b = (xbuttons & XINPUT_GAMEPAD_B) != 0;
                state.buttons.x = (xbuttons & XINPUT_GAMEPAD_X) != 0;
                state.buttons.y = (xbuttons & XINPUT_GAMEPAD_Y) != 0;
                state.buttons.leftStick = (xbuttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
                state.buttons.rightStick = (xbuttons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
                state.buttons.leftShoulder = (xbuttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
                state.buttons.rightShoulder = (xbuttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
                state.buttons.back = (xbuttons & XINPUT_GAMEPAD_BACK) != 0;
                state.buttons.start = (xbuttons & XINPUT_GAMEPAD_START) != 0;

                state.dpad.up = (xbuttons & XINPUT_GAMEPAD_DPAD_UP) != 0;
                state.dpad.down = (xbuttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
                state.dpad.right = (xbuttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
                state.dpad.left = (xbuttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;

                if (deadZoneMode == DEAD_ZONE_NONE)
                {
                    state.triggers.left = ApplyLinearDeadZone(float(xstate.Gamepad.bLeftTrigger), 255.f, 0.f);
                    state.triggers.right = ApplyLinearDeadZone(float(xstate.Gamepad.bRightTrigger), 255.f, 0.f);
                }
                else
                {
                    state.triggers.left = ApplyLinearDeadZone(float(xstate.Gamepad.bLeftTrigger), 255.f, float(XINPUT_GAMEPAD_TRIGGER_THRESHOLD));
                    state.triggers.right = ApplyLinearDeadZone(float(xstate.Gamepad.bRightTrigger), 255.f, float(XINPUT_GAMEPAD_TRIGGER_THRESHOLD));
                }

                ApplyStickDeadZone(float(xstate.Gamepad.sThumbLX), float(xstate.Gamepad.sThumbLY),
                                   deadZoneMode, 32767.f, float(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),
                                   state.thumbSticks.leftX, state.thumbSticks.leftY);

                ApplyStickDeadZone(float(xstate.Gamepad.sThumbRX), float(xstate.Gamepad.sThumbRY),
                                   deadZoneMode, 32767.f, float(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE),
                                   state.thumbSticks.rightX, state.thumbSticks.rightY);

                return;
            }
        }

        memset(&state, 0, sizeof(State));
    }

    void GetCapabilities(int player, _Out_ Capabilities& caps)
    {
        if (player == -1)
            player = GetMostRecent();

        ULONGLONG time = GetTickCount64();

        if (!ThrottleRetry(player, time))
        {
            XINPUT_CAPABILITIES xcaps;
            DWORD result = XInputGetCapabilities(DWORD(player), 0, &xcaps);
            if (result == ERROR_DEVICE_NOT_CONNECTED)
            {
                ClearSlot(player, time);
            }
            else
            {
                if (!mConnected[player])
                    mLastReadTime[player] = time;

                mConnected[player] = true;

                caps.connected = true;
                caps.id = uint64_t(player);
                if (xcaps.Type == XINPUT_DEVTYPE_GAMEPAD)
                {
                    static_assert(Capabilities::GAMEPAD == XINPUT_DEVSUBTYPE_GAMEPAD, "xinput.h mismatch");
                #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
                    static_assert(XINPUT_DEVSUBTYPE_WHEEL == Capabilities::WHEEL, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_ARCADE_STICK == Capabilities::ARCADE_STICK, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_FLIGHT_STICK == Capabilities::FLIGHT_STICK, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_DANCE_PAD == Capabilities::DANCE_PAD, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_GUITAR == Capabilities::GUITAR, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE == Capabilities::GUITAR_ALTERNATE, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_DRUM_KIT == Capabilities::DRUM_KIT, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_GUITAR_BASS == Capabilities::GUITAR_BASS, "xinput.h mismatch");
                    static_assert(XINPUT_DEVSUBTYPE_ARCADE_PAD == Capabilities::ARCADE_PAD, "xinput.h mismatch");
                #endif

                    caps.gamepadType = Capabilities::Type(xcaps.SubType);
                }

                // Hard-coded VID/PID
                caps.vid = 0x045E;
            #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
                caps.pid = (xcaps.Flags & XINPUT_CAPS_WIRELESS) ? 0x0719 : 0;
            #else
                caps.pid = 0;
            #endif

                return;
            }
        }

        memset(&caps, 0, sizeof(Capabilities));
    }

    bool SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger)
    {
        if (player == -1)
            player = GetMostRecent();

        ULONGLONG time = GetTickCount64();

        if (ThrottleRetry(player, time))
        {
            return false;
        }

        // XInput does not provide a way to set the left/right trigger impulse motors on the Xbox One Controller,
        // and these motors are not present on the Xbox 360 Common Controller
        UNREFERENCED_PARAMETER(leftTrigger);
        UNREFERENCED_PARAMETER(rightTrigger);

    #if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
        mLeftMotor[player] = leftMotor;
        mRightMotor[player] = rightMotor;

        if (mSuspended)
            return mConnected[player];
    #endif

        XINPUT_VIBRATION xvibration;
        xvibration.wLeftMotorSpeed = WORD(leftMotor * 0xFFFF);
        xvibration.wRightMotorSpeed = WORD(rightMotor * 0xFFFF);
        DWORD result = XInputSetState(DWORD(player), &xvibration);
        if (result == ERROR_DEVICE_NOT_CONNECTED)
        {
            ClearSlot(player, time);
            return false;
        }
        else
        {
            if (!mConnected[player])
                mLastReadTime[player] = time;

            mConnected[player] = true;
            return (result == ERROR_SUCCESS);
        }
    }

    void Suspend()
    {
    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        XInputEnable(FALSE);
    #else
            // For XInput 9.1.0, we have to emulate the behavior of XInputEnable( FALSE )
        if (!mSuspended)
        {
            for (size_t j = 0; j < XUSER_MAX_COUNT; ++j)
            {
                if (mConnected[j])
                {
                    XINPUT_VIBRATION xvibration;
                    xvibration.wLeftMotorSpeed = xvibration.wRightMotorSpeed = 0;
                    (void)XInputSetState(DWORD(j), &xvibration);
                }
            }

            mSuspended = true;
        }
    #endif
    }

    void Resume()
    {
    #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        XInputEnable(TRUE);
    #else
            // For XInput 9.1.0, we have to emulate the behavior of XInputEnable( TRUE )
        if (mSuspended)
        {
            ULONGLONG time = GetTickCount64();

            for (int j = 0; j < XUSER_MAX_COUNT; ++j)
            {
                if (mConnected[j])
                {
                    XINPUT_VIBRATION xvibration;
                    xvibration.wLeftMotorSpeed = WORD(mLeftMotor[j] * 0xFFFF);
                    xvibration.wRightMotorSpeed = WORD(mRightMotor[j] * 0xFFFF);
                    DWORD result = XInputSetState(DWORD(j), &xvibration);
                    if (result == ERROR_DEVICE_NOT_CONNECTED)
                    {
                        ClearSlot(j, time);
                    }
                }
            }

            mSuspended = false;
        }
    #endif
    }

    GamePad*    mOwner;

    static GamePad::Impl* s_gamePad;

private:
    bool        mConnected[XUSER_MAX_COUNT];
    ULONGLONG   mLastReadTime[XUSER_MAX_COUNT];

#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    // Variables for emulating XInputEnable on XInput 9.1.0
    float       mLeftMotor[XUSER_MAX_COUNT];
    float       mRightMotor[XUSER_MAX_COUNT];
    bool        mSuspended;
#endif

    bool ThrottleRetry(int player, ULONGLONG time)
    {
        // This function minimizes a potential performance issue with XInput on Windows when
        // checking a disconnected controller slot which requires device enumeration.
        // This throttling keeps checks for newly connected gamepads to about once a second

        if ((player < 0) || (player >= XUSER_MAX_COUNT))
            return true;

        if (mConnected[player])
            return false;

        for (int j = 0; j < XUSER_MAX_COUNT; ++j)
        {
            if (!mConnected[j])
            {
                LONGLONG delta = LONGLONG(time) - LONGLONG(mLastReadTime[j]);

                LONGLONG interval = 1000;
                if (j != player)
                    interval /= 4;

                if ((delta >= 0) && (delta < interval))
                    return true;
            }
        }

        return false;
    }

    void ClearSlot(int player, ULONGLONG time)
    {
        mConnected[player] = false;
        mLastReadTime[player] = time;
    #if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
        mLeftMotor[player] = mRightMotor[player] = 0.f;
    #endif
    }

    int GetMostRecent()
    {
        int player = -1;
        ULONGLONG time = 0;

        for (size_t j = 0; j < XUSER_MAX_COUNT; ++j)
        {
            if (mConnected[j] && (mLastReadTime[j] > time))
            {
                time = mLastReadTime[j];
                player = static_cast<int>(j);
            }
        }

        return player;
    }
};

GamePad::Impl* GamePad::Impl::s_gamePad = nullptr;

#endif

#pragma warning( disable : 4355 )

// Public constructor.
GamePad::GamePad() noexcept(false)
    : pImpl(std::make_unique<Impl>(this))
{
}


// Move constructor.
GamePad::GamePad(GamePad&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
    pImpl->mOwner = this;
}


// Move assignment.
GamePad& GamePad::operator= (GamePad&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    pImpl->mOwner = this;
    return *this;
}


// Public destructor.
GamePad::~GamePad()
{
}


GamePad::State GamePad::GetState(int player, DeadZone deadZoneMode)
{
    State state;
    pImpl->GetState(player, state, deadZoneMode);
    return state;
}


GamePad::Capabilities GamePad::GetCapabilities(int player)
{
    Capabilities caps;
    pImpl->GetCapabilities(player, caps);
    return caps;
}


bool GamePad::SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger)
{
    return pImpl->SetVibration(player, leftMotor, rightMotor, leftTrigger, rightTrigger);
}


void GamePad::Suspend()
{
    pImpl->Suspend();
}


void GamePad::Resume()
{
    pImpl->Resume();
}


#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10) || defined(_XBOX_ONE)
void GamePad::RegisterEvents(HANDLE ctrlChanged, HANDLE userChanged)
{
    pImpl->mCtrlChanged = (!ctrlChanged) ? INVALID_HANDLE_VALUE : ctrlChanged;
    pImpl->mUserChanged = (!userChanged) ? INVALID_HANDLE_VALUE : userChanged;
}
#endif


GamePad& GamePad::Get()
{
    if (!Impl::s_gamePad || !Impl::s_gamePad->mOwner)
        throw std::exception("GamePad is a singleton");

    return *Impl::s_gamePad->mOwner;
}



//======================================================================================
// ButtonStateTracker
//======================================================================================

#define UPDATE_BUTTON_STATE(field) field = static_cast<ButtonState>( ( !!state.buttons.field ) | ( ( !!state.buttons.field ^ !!lastState.buttons.field ) << 1 ) );

void GamePad::ButtonStateTracker::Update(const GamePad::State& state)
{
    UPDATE_BUTTON_STATE(a)

    assert((!state.buttons.a && !lastState.buttons.a) == (a == UP));
    assert((state.buttons.a && lastState.buttons.a) == (a == HELD));
    assert((!state.buttons.a && lastState.buttons.a) == (a == RELEASED));
    assert((state.buttons.a && !lastState.buttons.a) == (a == PRESSED));

    UPDATE_BUTTON_STATE(b)
    UPDATE_BUTTON_STATE(x)
    UPDATE_BUTTON_STATE(y)

    UPDATE_BUTTON_STATE(leftStick)
    UPDATE_BUTTON_STATE(rightStick)

    UPDATE_BUTTON_STATE(leftShoulder)
    UPDATE_BUTTON_STATE(rightShoulder)

    UPDATE_BUTTON_STATE(back)
    UPDATE_BUTTON_STATE(start)

    dpadUp = static_cast<ButtonState>((!!state.dpad.up) | ((!!state.dpad.up ^ !!lastState.dpad.up) << 1));
    dpadDown = static_cast<ButtonState>((!!state.dpad.down) | ((!!state.dpad.down ^ !!lastState.dpad.down) << 1));
    dpadLeft = static_cast<ButtonState>((!!state.dpad.left) | ((!!state.dpad.left ^ !!lastState.dpad.left) << 1));
    dpadRight = static_cast<ButtonState>((!!state.dpad.right) | ((!!state.dpad.right ^ !!lastState.dpad.right) << 1));

    assert((!state.dpad.up && !lastState.dpad.up) == (dpadUp == UP));
    assert((state.dpad.up && lastState.dpad.up) == (dpadUp == HELD));
    assert((!state.dpad.up && lastState.dpad.up) == (dpadUp == RELEASED));
    assert((state.dpad.up && !lastState.dpad.up) == (dpadUp == PRESSED));

    // Handle 'threshold' tests which emulate buttons

    bool threshold = state.IsLeftThumbStickUp();
    leftStickUp = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickUp()) << 1));

    threshold = state.IsLeftThumbStickDown();
    leftStickDown = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickDown()) << 1));

    threshold = state.IsLeftThumbStickLeft();
    leftStickLeft = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickLeft()) << 1));

    threshold = state.IsLeftThumbStickRight();
    leftStickRight = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickRight()) << 1));

    threshold = state.IsRightThumbStickUp();
    rightStickUp = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickUp()) << 1));

    threshold = state.IsRightThumbStickDown();
    rightStickDown = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickDown()) << 1));

    threshold = state.IsRightThumbStickLeft();
    rightStickLeft = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickLeft()) << 1));

    threshold = state.IsRightThumbStickRight();
    rightStickRight = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickRight()) << 1));

    threshold = state.IsLeftTriggerPressed();
    leftTrigger = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftTriggerPressed()) << 1));

    threshold = state.IsRightTriggerPressed();
    rightTrigger = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightTriggerPressed()) << 1));

    lastState = state;
}

#undef UPDATE_BUTTON_STATE


void GamePad::ButtonStateTracker::Reset() noexcept
{
    memset(this, 0, sizeof(ButtonStateTracker));
}
