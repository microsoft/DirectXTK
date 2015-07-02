//--------------------------------------------------------------------------------------
// File: Mouse.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Mouse.h"

#include "PlatformHelpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)

//======================================================================================
// Windows Store or universal Windows app implementation
//======================================================================================

//
// For a Windows Store app or universal Windows app, add the following to your existing
// application methods:
//
// void App::SetWindow(CoreWindow^ window )
// {
//     m_mouse->SetWindow(window);
// }
// 
// void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
// {
//     m_mouse->SetDpi(sender->LogicalDpi);
// }
//

#ifndef __cplusplus_winrt
#error This implementation requires C++/CX (/ZW)
#endif

#include <Windows.Foundation.h>
#include <Windows.UI.Core.h>
#include <Windows.Devices.Input.h>

class Mouse::Impl
{
public:
    Impl(Mouse* owner) :
        mOwner(owner),
        mDPI(96.f)
    {
        mPointerPressedToken.value = 0;
        mPointerReleasedToken.value = 0;
        mPointerMovedToken.value = 0;
        mPointerWheelToken.value = 0;
        
        if ( s_mouse )
        {
            throw std::exception( "Mouse is a singleton" );
        }

        s_mouse = this;

        memset( &mState, 0, sizeof(State) );

        mScrollWheelValue.reset( CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE) );
        if ( !mScrollWheelValue )
        {
            throw std::exception( "CreateEventEx" );
        }
    }

    ~Impl()
    {
        s_mouse = nullptr;

        RemoveHandlers();
    }

    void GetState(State& state) const
    {
        memcpy( &state, &mState, sizeof(State) );

        DWORD result = WaitForSingleObjectEx( mScrollWheelValue.get(), 0, FALSE );
        if ( result == WAIT_FAILED )
            throw std::exception( "WaitForSingleObjectEx" );

        if ( result == WAIT_OBJECT_0 )
        {
            state.scrollWheelValue = 0;
        }
    }

    void ResetScrollWheelValue()
    {
        SetEvent(mScrollWheelValue.get());
    }

    void SetWindow(ABI::Windows::UI::Core::ICoreWindow* window)
    {
        using namespace Microsoft::WRL;
        using namespace Microsoft::WRL::Wrappers;

        if (mWindow.Get() == window)
            return;

        RemoveHandlers();

        mWindow = window;

        if (!window)
            return;

        typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs PointerHandler;
        auto cb = Callback<PointerHandler>(PointerEvent);

        HRESULT hr = window->add_PointerPressed(cb.Get(), &mPointerPressedToken);
        ThrowIfFailed(hr);

        hr = window->add_PointerReleased(cb.Get(), &mPointerReleasedToken);
        ThrowIfFailed(hr);

        hr = window->add_PointerMoved(Callback<PointerHandler>(PointerMoved).Get(), &mPointerMovedToken);
        ThrowIfFailed(hr);

        hr = window->add_PointerWheelChanged(Callback<PointerHandler>(PointerWheel).Get(), &mPointerWheelToken);
        ThrowIfFailed(hr);
    }

    State           mState;
    float           mDPI;
    Mouse*          mOwner;

    static Mouse::Impl* s_mouse;

private:
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> mWindow;

    ScopedHandle    mScrollWheelValue;

    EventRegistrationToken mPointerPressedToken;
    EventRegistrationToken mPointerReleasedToken;
    EventRegistrationToken mPointerMovedToken;
    EventRegistrationToken mPointerWheelToken;

    void RemoveHandlers()
    {
        if (mWindow)
        {
            mWindow->remove_PointerPressed(mPointerPressedToken);
            mPointerPressedToken.value = 0;

            mWindow->remove_PointerReleased(mPointerReleasedToken);
            mPointerReleasedToken.value = 0;

            mWindow->remove_PointerMoved(mPointerMovedToken);
            mPointerMovedToken.value = 0;

            mWindow->remove_PointerWheelChanged(mPointerWheelToken);
            mPointerWheelToken.value = 0;
        }
    }

    static HRESULT PointerEvent( IInspectable *, ABI::Windows::UI::Core::IPointerEventArgs*args )
    {
        if (!s_mouse)
            return S_OK;

        ComPtr<ABI::Windows::UI::Input::IPointerPoint> currentPoint;
        HRESULT hr = args->get_CurrentPoint( currentPoint.GetAddressOf() );
        ThrowIfFailed(hr);

        ABI::Windows::Foundation::Point pos;
        hr = currentPoint->get_Position( &pos );
        ThrowIfFailed(hr);

        float dpi = s_mouse->mDPI;

        s_mouse->mState.x = static_cast<int>( pos.X * dpi / 96.f + 0.5f );
        s_mouse->mState.y = static_cast<int>( pos.Y * dpi / 96.f + 0.5f );

        ComPtr<ABI::Windows::Devices::Input::IPointerDevice> pointerDevice;
        hr = currentPoint->get_PointerDevice( pointerDevice.GetAddressOf() );
        ThrowIfFailed(hr);

        ABI::Windows::Devices::Input::PointerDeviceType devType;
        hr = pointerDevice->get_PointerDeviceType( &devType );
        ThrowIfFailed(hr);

        if (devType == ABI::Windows::Devices::Input::PointerDeviceType::PointerDeviceType_Mouse)
        {
            ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> props;
            hr = currentPoint->get_Properties( props.GetAddressOf() );
            ThrowIfFailed(hr);

            boolean value;
            hr = props->get_IsLeftButtonPressed(&value);
            ThrowIfFailed(hr);
            s_mouse->mState.leftButton = value != 0;

            hr = props->get_IsRightButtonPressed(&value);
            ThrowIfFailed(hr);
            s_mouse->mState.rightButton = value != 0;

            hr = props->get_IsMiddleButtonPressed(&value);
            ThrowIfFailed(hr);
            s_mouse->mState.middleButton = value != 0;

            hr = props->get_IsXButton1Pressed(&value);
            ThrowIfFailed(hr);
            s_mouse->mState.xButton1 = value != 0;

            hr = props->get_IsXButton2Pressed(&value);
            ThrowIfFailed(hr);
            s_mouse->mState.xButton2 = value != 0;
        }
        return S_OK;
    }

    static HRESULT PointerMoved(IInspectable *, ABI::Windows::UI::Core::IPointerEventArgs* args)
    {
        if (!s_mouse)
            return S_OK;

        ComPtr<ABI::Windows::UI::Input::IPointerPoint> currentPoint;
        HRESULT hr = args->get_CurrentPoint(currentPoint.GetAddressOf());
        ThrowIfFailed(hr);

        ABI::Windows::Foundation::Point pos;
        hr = currentPoint->get_Position(&pos);
        ThrowIfFailed(hr);

        float dpi = s_mouse->mDPI;

        s_mouse->mState.x = static_cast<int>(pos.X * dpi / 96.f + 0.5f);
        s_mouse->mState.y = static_cast<int>(pos.Y * dpi / 96.f + 0.5f);

        return S_OK;
    }

    static HRESULT PointerWheel( IInspectable *, ABI::Windows::UI::Core::IPointerEventArgs*args )
    {
        if (!s_mouse)
            return S_OK;

        ComPtr<ABI::Windows::UI::Input::IPointerPoint> currentPoint;
        HRESULT hr = args->get_CurrentPoint( currentPoint.GetAddressOf() );
        ThrowIfFailed(hr);

        ABI::Windows::Foundation::Point pos;
        hr = currentPoint->get_Position( &pos );
        ThrowIfFailed(hr);

        float dpi = s_mouse->mDPI;

        s_mouse->mState.x = static_cast<int>( pos.X * dpi / 96.f + 0.5f );
        s_mouse->mState.y = static_cast<int>( pos.Y * dpi / 96.f + 0.5f );

        ComPtr<ABI::Windows::Devices::Input::IPointerDevice> pointerDevice;
        hr = currentPoint->get_PointerDevice( pointerDevice.GetAddressOf() );
        ThrowIfFailed(hr);

        ABI::Windows::Devices::Input::PointerDeviceType devType;
        hr = pointerDevice->get_PointerDeviceType( &devType );
        ThrowIfFailed(hr);

        if (devType == ABI::Windows::Devices::Input::PointerDeviceType::PointerDeviceType_Mouse)
        {
            ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> props;
            hr = currentPoint->get_Properties( props.GetAddressOf() );
            ThrowIfFailed(hr);

            INT32 value;
            hr = props->get_MouseWheelDelta(&value);
            ThrowIfFailed(hr);

            HANDLE evt = s_mouse->mScrollWheelValue.get();
            if (WaitForSingleObjectEx(evt, 0, FALSE) == WAIT_OBJECT_0)
            {
                s_mouse->mState.scrollWheelValue = 0;
                ResetEvent(evt);
            }

            s_mouse->mState.scrollWheelValue += value;
        }

        return S_OK;
    }
};


Mouse::Impl* Mouse::Impl::s_mouse = nullptr;


void Mouse::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
    // See https://msdn.microsoft.com/en-us/library/hh755802.aspx
    auto iwindow = reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window);
    pImpl->SetWindow(iwindow);
}


void Mouse::SetDpi(float dpi)
{
    auto pImpl = Impl::s_mouse;

    if (!pImpl)
        return;

    pImpl->mDPI = dpi;
}


#elif defined(_XBOX_ONE) || ( defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP) )

//======================================================================================
// Null device for Windows Phone and Xbox One
//======================================================================================

class Mouse::Impl
{
public:
    Impl(Mouse* owner) :
        mOwner(owner)
    {
        if ( s_mouse )
        {
            throw std::exception( "Mouse is a singleton" );
        }

        s_mouse = this;
    }

    ~Impl()
    {
        s_mouse = nullptr;
    }

    void GetState(State& state) const
    {
        memset( &state, 0, sizeof(State) );
    }

    void ResetScrollWheelValue()
    {
    }

    Mouse*  mOwner;

    static Mouse::Impl* s_mouse;
};

Mouse::Impl* Mouse::Impl::s_mouse = nullptr;

#else

//======================================================================================
// Win32 desktop implementation
//======================================================================================

//
// For a Win32 desktop application, call this static function from your Window Message Procedure
//
// LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// {
//     switch (message)
//     {
//     case WM_MOUSEMOVE:
//     case WM_LBUTTONDOWN:
//     case WM_LBUTTONUP:
//     case WM_RBUTTONDOWN:
//     case WM_RBUTTONUP:
//     case WM_MBUTTONDOWN:
//     case WM_MBUTTONUP:
//     case WM_MOUSEWHEEL:
//     case WM_XBUTTONDOWN:
//     case WM_XBUTTONUP:
//         Mouse::ProcessMessage(message, wParam, lParam);
//         break;
//
//     }
// }
//

class Mouse::Impl
{
public:
    Impl(Mouse* owner) :
        mOwner(owner)
    {
        if ( s_mouse )
        {
            throw std::exception( "Mouse is a singleton" );
        }

        s_mouse = this;

        memset( &mState, 0, sizeof(State) );

        mScrollWheelValue.reset( CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE) );
        if ( !mScrollWheelValue )
        {
            throw std::exception( "CreateEventEx" );
        }
    }

    ~Impl()
    {
        s_mouse = nullptr;
    }

    void GetState(State& state) const
    {
        memcpy( &state, &mState, sizeof(State) );

        DWORD result = WaitForSingleObjectEx( mScrollWheelValue.get(), 0, FALSE );
        if ( result == WAIT_FAILED )
            throw std::exception( "WaitForSingleObjectEx" );

        if ( result == WAIT_OBJECT_0 )
        {
            state.scrollWheelValue = 0;
        }
    }

    void ResetScrollWheelValue()
    {
        SetEvent(mScrollWheelValue.get());
    }

    State           mState;

    Mouse*          mOwner;

    static Mouse::Impl* s_mouse;

private:
    ScopedHandle    mScrollWheelValue;

    friend void Mouse::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);
};


Mouse::Impl* Mouse::Impl::s_mouse = nullptr;


void Mouse::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    auto pImpl = Impl::s_mouse;

    if (!pImpl)
        return;

    HANDLE evt = pImpl->mScrollWheelValue.get();
    if (WaitForSingleObjectEx(evt, 0, FALSE) == WAIT_OBJECT_0)
    {
        pImpl->mState.scrollWheelValue = 0;
        ResetEvent(evt);
    }

    switch (message)
    {
    case WM_MOUSEMOVE:
        break;

    case WM_LBUTTONDOWN:
        pImpl->mState.leftButton = true;
        break;

    case WM_LBUTTONUP:
        pImpl->mState.leftButton = false;
        break;

    case WM_RBUTTONDOWN:
        pImpl->mState.rightButton = true;
        break;

    case WM_RBUTTONUP:
        pImpl->mState.rightButton = false;
        break;

    case WM_MBUTTONDOWN:
        pImpl->mState.middleButton = true;
        break;

    case WM_MBUTTONUP:
        pImpl->mState.middleButton = false;
        break;

    case WM_MOUSEWHEEL:
        pImpl->mState.scrollWheelValue += GET_WHEEL_DELTA_WPARAM(wParam);
        return;

    case WM_XBUTTONDOWN:
        switch (GET_XBUTTON_WPARAM(wParam))
        {
        case XBUTTON1:
            pImpl->mState.xButton1 = true;
            break;

        case XBUTTON2:
            pImpl->mState.xButton2 = true;
            break;
        }
        break;

    case WM_XBUTTONUP:
        switch (GET_XBUTTON_WPARAM(wParam))
        {
        case XBUTTON1:
            pImpl->mState.xButton1 = false;
            break;

        case XBUTTON2:
            pImpl->mState.xButton2 = false;
            break;
        }
        break;

    default:
        // Not a mouse message, so exit
        return;
    }

    // All mouse messages provide a new pointer position
    pImpl->mState.x = static_cast<short>( LOWORD(lParam) ); // GET_X_LPARAM(lParam); 
    pImpl->mState.y = static_cast<short>( HIWORD(lParam) ); // GET_Y_LPARAM(lParam); 
}

#endif

#pragma warning( disable : 4355 )

// Public constructor.
Mouse::Mouse()
    : pImpl( new Impl(this) )
{
}


// Move constructor.
Mouse::Mouse(Mouse&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
    pImpl->mOwner = this;
}


// Move assignment.
Mouse& Mouse::operator= (Mouse&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    pImpl->mOwner = this;
    return *this;
}


// Public destructor.
Mouse::~Mouse()
{
}


Mouse::State Mouse::GetState() const
{
    State state;
    pImpl->GetState(state);
    return state;
}


void Mouse::ResetScrollWheelValue()
{
    pImpl->ResetScrollWheelValue();
}


Mouse& Mouse::Get()
{
    if ( !Impl::s_mouse || !Impl::s_mouse->mOwner )
        throw std::exception( "Mouse is a singleton" );

    return *Impl::s_mouse->mOwner;
}



//======================================================================================
// ButtonStateTracker
//======================================================================================

#define UPDATE_BUTTON_STATE(field) field = static_cast<ButtonState>( ( !!state.field ) | ( ( !!state.field ^ !!lastState.field ) << 1 ) );

void Mouse::ButtonStateTracker::Update( const Mouse::State& state )
{
    UPDATE_BUTTON_STATE(leftButton);

    assert( ( !state.leftButton && !lastState.leftButton ) == ( leftButton == UP ) );
    assert( ( state.leftButton && lastState.leftButton ) == ( leftButton == HELD ) );
    assert( ( !state.leftButton && lastState.leftButton ) == ( leftButton == RELEASED ) );
    assert( ( state.leftButton && !lastState.leftButton ) == ( leftButton == PRESSED ) );

    UPDATE_BUTTON_STATE(middleButton);
    UPDATE_BUTTON_STATE(rightButton);
    UPDATE_BUTTON_STATE(xButton1);
    UPDATE_BUTTON_STATE(xButton2);

    lastState = state;
}

#undef UPDATE_BUTTON_STATE


void Mouse::ButtonStateTracker::Reset()
{
    memset( this, 0, sizeof(ButtonStateTracker) );
}
