//--------------------------------------------------------------------------------------
// File: PlatformHelpers.h
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

#pragma once

#include <exception>

#if _WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/
#include "atlbase.h"
#else
#include <wrl.h>
#endif


// Emulate SAL2 macros when building with Visual Studio versions < 2011.
#if defined(_MSC_VER) && (_MSC_VER < 1610) && !defined (_Outptr_)
#define _Outptr_
#define _Out_writes_all_(c)
#define _Inout_updates_(c)
#endif


namespace DirectX
{
    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }
}


#if _WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/

// Emulate the new Metro ComPtr type via ATL CComPtr when building for Windows versions prior to Win8.
namespace Microsoft
{
    namespace WRL
    {
        template<typename T>
        class ComPtr : public CComPtr<T>
        {
        public:
            ComPtr()
            { }

            ComPtr(_In_opt_ T* value)
              : CComPtr(value)
            { }

            T* Get() const
            {
                return *this;
            }

            template<typename Q>
            HRESULT As(_Outptr_ Q** result) const
            {
                return QueryInterface(result);
            }
        };
    }
}

#endif


#if defined(_MSC_VER) && (_MSC_VER < 1610)

// Emulate the C++0x mutex and lock_guard types when building with Visual Studio versions < 2011.
namespace std
{
    class mutex
    {
    public:
        mutex()         { InitializeCriticalSection(&mCriticalSection); }
        ~mutex()        { DeleteCriticalSection(&mCriticalSection); }

        void lock()     { EnterCriticalSection(&mCriticalSection); }
        void unlock()   { LeaveCriticalSection(&mCriticalSection); }
        bool try_lock() { return TryEnterCriticalSection(&mCriticalSection) != 0; }

    private:
        CRITICAL_SECTION mCriticalSection;

        mutex(mutex const&);
        mutex& operator= (mutex const&);
    };


    template<typename Mutex>
    class lock_guard
    {
    public:
        typedef Mutex mutex_type;

        explicit lock_guard(mutex_type& mutex)
          : mMutex(mutex)
        {
            mMutex.lock();
        }

        ~lock_guard()
        {
            mMutex.unlock();
        }

    private:
        mutex_type& mMutex;

        lock_guard(lock_guard const&);
        lock_guard& operator= (lock_guard const&);
    };
}

#else   // _MSC_VER < 1610

#include <mutex>

#endif
