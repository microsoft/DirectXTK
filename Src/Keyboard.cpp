//--------------------------------------------------------------------------------------
// File: Keyboard.cpp
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
#include "Keyboard.h"

using namespace DirectX;

static_assert(sizeof(Keyboard::State) == (256 / 8), "Size mismatch for State");

Keyboard::State Keyboard::s_state = { 0 };


//--------------------------------------------------------------------------------------
void DirectX::Keyboard::KeyDown( int key )
{
    if (key < 0 || key > 0xfe)
        return;

    auto ptr = reinterpret_cast<uint32_t*>(&s_state);

    unsigned int bf = 1u << (key & 0x1f);
    ptr[(key >> 5)] |= bf;
}

void DirectX::Keyboard::KeyUp( int key )
{
    if (key < 0 || key > 0xfe)
        return;

    auto ptr = reinterpret_cast<uint32_t*>(&s_state);

    unsigned int bf = 1u << (key & 0x1f);
    ptr[(key >> 5)] &= ~bf;
}



//--------------------------------------------------------------------------------------
// Win32 desktop application implementation
//--------------------------------------------------------------------------------------

//
// For a Win32 desktop application, call this function from your Window Message Procedure
//
// LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// {
//     switch (message)
//     {
//
//     case WM_ACTIVATEAPP:
//         Keyboard::ProcessMessage(message, wParam, lParam);
//         break;
//
//     case WM_KEYDOWN:
//     case WM_SYSKEYDOWN:
//     case WM_KEYUP:
//     case WM_SYSKEYUP:
//         Keyboard::ProcessMessage(message, wParam, lParam);
//         break;
//
//     }
// }

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

void DirectX::Keyboard::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool down = false;

    switch (message)
    {
    case WM_ACTIVATEAPP:
        Reset();
        return;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = true;
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        break;

    default:
        return;
    }

    int vk = static_cast<int>( wParam );
    switch (vk)
    {
    case VK_SHIFT:
        vk = MapVirtualKey( (lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX );
        break;

    case VK_CONTROL:
        vk = (lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
        break;

    case VK_MENU:
        vk = (lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
        break;
    }

    if (vk >= 0 && vk < 0xff)
    {
        auto ptr = reinterpret_cast<uint32_t*>(&s_state);

        unsigned int bf = 1u << (vk & 0x1f);
        if (down)
        {
            ptr[(vk >> 5)] |= bf;
        }
        else
        {
            ptr[(vk >> 5)] &= ~bf;
        }
    }
}
#endif


//--------------------------------------------------------------------------------------
// ICoreWindow implementation
//--------------------------------------------------------------------------------------
//
// For a Windows Store app or universal Windows app, register for these two events and handle them as follows:
//
// window->Dispatcher->AcceleratorKeyActivated += ref new TypedEventHandler<CoreDispatcher^, AcceleratorKeyEventArgs^>(this, &App::OnAcceleratorKeyActivated);
// window->Activated += ref new TypedEventHandler <CoreWindow ^, WindowActivatedEventArgs^>(this, &App::OnWindowActivated);
//
// void App::OnAcceleratorKeyActivated(CoreDispatcher^ sender, AcceleratorKeyEventArgs^ args)
// {
//     Keyboard::ProcessAcceleratorKeyEvent(sender, args);
// }
//
// void App::OnWindowActivated(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args)
// {
//     Keyboard::Reset();
// }
//

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)

#ifndef __cplusplus_winrt
#error This implementation requires C++/CX (/ZW)
#endif

void DirectX::Keyboard::ProcessAcceleratorKeyEvent(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args)
{
    UNREFERENCED_PARAMETER(sender);

    using namespace Windows::System;
    using namespace Windows::UI::Core;

    CoreAcceleratorKeyEventType event = args->EventType;

    bool down = false;

    switch (event)
    {
    case CoreAcceleratorKeyEventType::KeyDown:
    case CoreAcceleratorKeyEventType::SystemKeyDown:
        down = true;
        break;

    case CoreAcceleratorKeyEventType::KeyUp:
    case CoreAcceleratorKeyEventType::SystemKeyUp:
        break;

    default:
        return;
    }

    int vk = static_cast<int>( args->VirtualKey );

    switch (vk)
    {
    case VK_SHIFT:
        vk = (args->KeyStatus.ScanCode == 0x36) ? VK_RSHIFT : VK_LSHIFT;
        break;

    case VK_CONTROL:
        vk = (args->KeyStatus.IsExtendedKey) ? VK_RCONTROL : VK_LCONTROL;
        break;

    case VK_MENU:
        vk = (args->KeyStatus.IsExtendedKey) ? VK_RMENU : VK_LMENU;
        break;
    }

    if (vk >= 0 && vk < 0xff)
    {
        auto ptr = reinterpret_cast<uint32_t*>(&s_state);

        unsigned int bf = 1u << (vk & 0x1f);
        if (down)
        {
            ptr[(vk >> 5)] |= bf;
        }
        else
        {
            ptr[(vk >> 5)] &= ~bf;
        }
    }
}
#endif


//======================================================================================
// KeyboardStateTracker
//======================================================================================

void DirectX::Keyboard::KeyboardStateTracker::Update( const State& state )
{
    auto currPtr = reinterpret_cast<const uint32_t*>(&state);
    auto prevPtr = reinterpret_cast<const uint32_t*>(&lastState);
    auto releasedPtr = reinterpret_cast<uint32_t*>(&released);
    auto pressedPtr = reinterpret_cast<uint32_t*>(&pressed);
    for (size_t j = 0; j < (256 / 32); ++j)
    {
        *pressedPtr = *currPtr & ~(*prevPtr);
        *releasedPtr = ~(*currPtr) & *prevPtr;

        ++currPtr;
        ++prevPtr;
        ++releasedPtr;
        ++pressedPtr;
    }

    lastState = state;
}
