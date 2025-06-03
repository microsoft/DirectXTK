//--------------------------------------------------------------------------------------
// File: Audio.h
//
// DirectXTK for Audio header
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <objbase.h>
#include <mmreg.h>
#include <Audioclient.h>

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
#include <xma2defs.h>
#ifdef _MSC_VER
#pragma comment(lib,"acphal.lib")
#endif
#endif

#ifndef XAUDIO2_HELPER_FUNCTIONS
#define XAUDIO2_HELPER_FUNCTIONS
#endif

#if defined(USING_XAUDIO2_REDIST) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/) || defined(_XBOX_ONE)
#define USING_XAUDIO2_9
#elif (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
#define USING_XAUDIO2_8
#else
#error DirectX Tool Kit for Audio not supported on this platform
#endif

#include <xaudio2.h>
#include <xaudio2fx.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4619 4616 5246)
#endif
#include <x3daudio.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <xapofx.h>

#if !defined(USING_XAUDIO2_REDIST) && defined(_MSC_VER)
#if defined(USING_XAUDIO2_8) && defined(NTDDI_WIN10) && !defined(_M_IX86)
// The xaudio2_8.lib in the Windows 10 SDK for x86 is incorrectly annotated as __cdecl instead of __stdcall, so avoid using it in this case.
#pragma comment(lib,"xaudio2_8.lib")
#else
#pragma comment(lib,"xaudio2.lib")
#endif
#endif

#include <DirectXMath.h>

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


namespace DirectX
{
    class SoundEffectInstance;
    class SoundStreamInstance;

    //----------------------------------------------------------------------------------
    struct AudioStatistics
    {
        size_t  playingOneShots;        // Number of one-shot sounds currently playing
        size_t  playingInstances;       // Number of sound effect instances currently playing
        size_t  allocatedInstances;     // Number of SoundEffectInstance allocated
        size_t  allocatedVoices;        // Number of XAudio2 voices allocated (standard, 3D, one-shots, and idle one-shots)
        size_t  allocatedVoices3d;      // Number of XAudio2 voices allocated for 3D
        size_t  allocatedVoicesOneShot; // Number of XAudio2 voices allocated for one-shot sounds
        size_t  allocatedVoicesIdle;    // Number of XAudio2 voices allocated for one-shot sounds but not currently in use
        size_t  audioBytes;             // Total wave data (in bytes) in SoundEffects and in-memory WaveBanks
    #if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        size_t  xmaAudioBytes;          // Total wave data (in bytes) in SoundEffects and in-memory WaveBanks allocated with ApuAlloc
    #endif
        size_t  streamingBytes;         // Total size of streaming buffers (in bytes) in streaming WaveBanks
    };


    //----------------------------------------------------------------------------------
    class DIRECTX_TOOLKIT_API IVoiceNotify
    {
    public:
        virtual ~IVoiceNotify() = default;

        IVoiceNotify(const IVoiceNotify&) = delete;
        IVoiceNotify& operator=(const IVoiceNotify&) = delete;

        IVoiceNotify(IVoiceNotify&&) = default;
        IVoiceNotify& operator=(IVoiceNotify&&) = default;

        virtual void __cdecl OnBufferEnd() = 0;
            // Notfication that a voice buffer has finished
            // Note this is called from XAudio2's worker thread, so it should perform very minimal and thread-safe operations

        virtual void __cdecl OnCriticalError() = 0;
            // Notification that the audio engine encountered a critical error

        virtual void __cdecl OnReset() = 0;
            // Notification of an audio engine reset

        virtual void __cdecl OnUpdate() = 0;
            // Notification of an audio engine per-frame update (opt-in)

        virtual void __cdecl OnDestroyEngine() noexcept = 0;
            // Notification that the audio engine is being destroyed

        virtual void __cdecl OnTrim() = 0;
            // Notification of a request to trim the voice pool

        virtual void __cdecl GatherStatistics(AudioStatistics& stats) const = 0;
            // Contribute to statistics request

        virtual void __cdecl OnDestroyParent() noexcept = 0;
            // Optional notification used by some objects

    protected:
        IVoiceNotify() = default;
    };

    //----------------------------------------------------------------------------------
    enum AUDIO_ENGINE_FLAGS : uint32_t
    {
        AudioEngine_Default = 0x0,

        AudioEngine_EnvironmentalReverb = 0x1,
        AudioEngine_ReverbUseFilters = 0x2,
        AudioEngine_UseMasteringLimiter = 0x4,
        AudioEngine_DisableLFERedirect = 0x8,
        AudioEngine_DisableDopplerEffect = 0x10,
        AudioEngine_ZeroCenter3D = 0x20,

        AudioEngine_Debug = 0x10000,
        AudioEngine_ThrowOnNoAudioHW = 0x20000,
        AudioEngine_DisableVoiceReuse = 0x40000,
    };

    enum SOUND_EFFECT_INSTANCE_FLAGS : uint32_t
    {
        SoundEffectInstance_Default = 0x0,

        SoundEffectInstance_Use3D = 0x1,
        SoundEffectInstance_ReverbUseFilters = 0x2,
        SoundEffectInstance_NoSetPitch = 0x4,
        SoundEffectInstance_UseRedirectLFE = 0x8,
        SoundEffectInstance_ZeroCenter3D = 0x10,
    };

    enum AUDIO_ENGINE_REVERB : uint32_t
    {
        Reverb_Off,
        Reverb_Default,
        Reverb_Generic,
        Reverb_Forest,
        Reverb_PaddedCell,
        Reverb_Room,
        Reverb_Bathroom,
        Reverb_LivingRoom,
        Reverb_StoneRoom,
        Reverb_Auditorium,
        Reverb_ConcertHall,
        Reverb_Cave,
        Reverb_Arena,
        Reverb_Hangar,
        Reverb_CarpetedHallway,
        Reverb_Hallway,
        Reverb_StoneCorridor,
        Reverb_Alley,
        Reverb_City,
        Reverb_Mountains,
        Reverb_Quarry,
        Reverb_Plain,
        Reverb_ParkingLot,
        Reverb_SewerPipe,
        Reverb_Underwater,
        Reverb_SmallRoom,
        Reverb_MediumRoom,
        Reverb_LargeRoom,
        Reverb_MediumHall,
        Reverb_LargeHall,
        Reverb_Plate,
        Reverb_MAX
    };

    enum SoundState : uint32_t
    {
        STOPPED = 0,
        PLAYING,
        PAUSED
    };


    //----------------------------------------------------------------------------------
    class AudioEngine
    {
    public:
        DIRECTX_TOOLKIT_API explicit AudioEngine(
            AUDIO_ENGINE_FLAGS flags = AudioEngine_Default,
            _In_opt_ const WAVEFORMATEX* wfx = nullptr,
            _In_opt_z_ const wchar_t* deviceId = nullptr,
            AUDIO_STREAM_CATEGORY category = AudioCategory_GameEffects) noexcept(false);

        DIRECTX_TOOLKIT_API AudioEngine(AudioEngine&&) noexcept;
        DIRECTX_TOOLKIT_API AudioEngine& operator= (AudioEngine&&) noexcept;

        AudioEngine(AudioEngine const&) = delete;
        AudioEngine& operator= (AudioEngine const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~AudioEngine();

        DIRECTX_TOOLKIT_API bool __cdecl Update();
            // Performs per-frame processing for the audio engine, returns false if in 'silent mode'

        DIRECTX_TOOLKIT_API bool __cdecl Reset(
            _In_opt_ const WAVEFORMATEX* wfx = nullptr,
            _In_opt_z_ const wchar_t* deviceId = nullptr);
            // Reset audio engine from critical error/silent mode using a new device; can also 'migrate' the graph
            // Returns true if succesfully reset, false if in 'silent mode' due to no default device
            // Note: One shots are lost, all SoundEffectInstances are in the STOPPED state after successful reset

        DIRECTX_TOOLKIT_API void __cdecl Suspend() noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Resume();
            // Suspend/resumes audio processing (i.e. global pause/resume)

        DIRECTX_TOOLKIT_API float __cdecl GetMasterVolume() const noexcept;
        DIRECTX_TOOLKIT_API void __cdecl SetMasterVolume(float volume);
            // Master volume property for all sounds

        DIRECTX_TOOLKIT_API void __cdecl SetReverb(AUDIO_ENGINE_REVERB reverb);
        DIRECTX_TOOLKIT_API void __cdecl SetReverb(_In_opt_ const XAUDIO2FX_REVERB_PARAMETERS* native);
            // Sets environmental reverb for 3D positional audio (if active)

        DIRECTX_TOOLKIT_API void __cdecl SetMasteringLimit(int release, int loudness);
            // Sets the mastering volume limiter properties (if active)

        DIRECTX_TOOLKIT_API AudioStatistics __cdecl GetStatistics() const;
            // Gathers audio engine statistics

        DIRECTX_TOOLKIT_API WAVEFORMATEXTENSIBLE __cdecl GetOutputFormat() const noexcept;
            // Returns the format of the audio output device associated with the mastering voice.

        DIRECTX_TOOLKIT_API uint32_t __cdecl GetChannelMask() const noexcept;
            // Returns the output channel mask

        DIRECTX_TOOLKIT_API int __cdecl GetOutputSampleRate() const noexcept;
            // Returns the sample rate going into the mastering voice

        DIRECTX_TOOLKIT_API unsigned int __cdecl GetOutputChannels() const noexcept;
            // Returns the number of channels going into the mastering voice

        DIRECTX_TOOLKIT_API bool __cdecl IsAudioDevicePresent() const noexcept;
            // Returns true if the audio graph is operating normally, false if in 'silent mode'

        DIRECTX_TOOLKIT_API bool __cdecl IsCriticalError() const noexcept;
            // Returns true if the audio graph is halted due to a critical error (which also places the engine into 'silent mode')

        // Voice pool management.
        DIRECTX_TOOLKIT_API void __cdecl SetDefaultSampleRate(int sampleRate);
            // Sample rate for voices in the reuse pool (defaults to 44100)

        DIRECTX_TOOLKIT_API void __cdecl SetMaxVoicePool(size_t maxOneShots, size_t maxInstances);
            // Maximum number of voices to allocate for one-shots and instances
            // Note: one-shots over this limit are ignored; too many instance voices throws an exception

        DIRECTX_TOOLKIT_API void __cdecl TrimVoicePool();
            // Releases any currently unused voices

        // Internal-use functions
        void __cdecl AllocateVoice(_In_ const WAVEFORMATEX* wfx,
            SOUND_EFFECT_INSTANCE_FLAGS flags, bool oneshot,
            _Outptr_result_maybenull_ IXAudio2SourceVoice** voice);

        DIRECTX_TOOLKIT_API void __cdecl DestroyVoice(_In_ IXAudio2SourceVoice* voice) noexcept;
            // Should only be called for instance voices, not one-shots

        DIRECTX_TOOLKIT_API void __cdecl RegisterNotify(_In_ IVoiceNotify* notify, bool usesUpdate);
        void __cdecl UnregisterNotify(_In_ IVoiceNotify* notify, bool usesOneShots, bool usesUpdate);

        // XAudio2 interface access
        DIRECTX_TOOLKIT_API IXAudio2* __cdecl GetInterface() const noexcept;
        DIRECTX_TOOLKIT_API IXAudio2MasteringVoice* __cdecl GetMasterVoice() const noexcept;
        DIRECTX_TOOLKIT_API IXAudio2SubmixVoice* __cdecl GetReverbVoice() const noexcept;

        // X3DAudio interface access
        DIRECTX_TOOLKIT_API X3DAUDIO_HANDLE& __cdecl Get3DHandle() const noexcept;
        DIRECTX_TOOLKIT_API uint32_t __cdecl Get3DCalculateFlags() const noexcept;

        // Static functions
        struct RendererDetail
        {
            std::wstring deviceId;
            std::wstring description;
        };

        DIRECTX_TOOLKIT_API static std::vector<RendererDetail> __cdecl GetRendererDetails();
            // Returns a list of valid audio endpoint devices

    #if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
        DIRECTX_TOOLKIT_API explicit AudioEngine(
            AUDIO_ENGINE_FLAGS flags = AudioEngine_Default,
            _In_opt_ const WAVEFORMATEX* wfx = nullptr,
            _In_opt_z_ const __wchar_t* deviceId = nullptr,
            AUDIO_STREAM_CATEGORY category = AudioCategory_GameEffects) noexcept(false);

        DIRECTX_TOOLKIT_API bool __cdecl Reset(
            _In_opt_ const WAVEFORMATEX* wfx = nullptr,
            _In_opt_z_ const __wchar_t* deviceId = nullptr);
    #endif

    private:
        // Private implementation.
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    class WaveBank
    {
    public:
        DIRECTX_TOOLKIT_API WaveBank(
            _In_ AudioEngine* engine,
            _In_z_ const wchar_t* wbFileName);

        DIRECTX_TOOLKIT_API WaveBank(WaveBank&&) noexcept;
        DIRECTX_TOOLKIT_API WaveBank& operator= (WaveBank&&) noexcept;

        WaveBank(WaveBank const&) = delete;
        WaveBank& operator= (WaveBank const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~WaveBank();

        DIRECTX_TOOLKIT_API void __cdecl Play(unsigned int index);
        DIRECTX_TOOLKIT_API void __cdecl Play(
            unsigned int index,
            float volume, float pitch, float pan);

        DIRECTX_TOOLKIT_API void __cdecl Play(_In_z_ const char* name);
        DIRECTX_TOOLKIT_API void __cdecl Play(
            _In_z_ const char* name,
            float volume, float pitch, float pan);

        DIRECTX_TOOLKIT_API std::unique_ptr<SoundEffectInstance> __cdecl CreateInstance(
            unsigned int index,
            SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);
        DIRECTX_TOOLKIT_API std::unique_ptr<SoundEffectInstance> __cdecl CreateInstance(
            _In_z_ const char* name,
            SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);

        DIRECTX_TOOLKIT_API std::unique_ptr<SoundStreamInstance> __cdecl CreateStreamInstance(
            unsigned int index,
            SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);
        DIRECTX_TOOLKIT_API std::unique_ptr<SoundStreamInstance> __cdecl CreateStreamInstance(
            _In_z_ const char* name,
            SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);

        DIRECTX_TOOLKIT_API bool __cdecl IsPrepared() const noexcept;
        DIRECTX_TOOLKIT_API bool __cdecl IsInUse() const noexcept;
        DIRECTX_TOOLKIT_API bool __cdecl IsStreamingBank() const noexcept;
        DIRECTX_TOOLKIT_API bool __cdecl IsAdvancedFormat() const noexcept;

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleSizeInBytes(unsigned int index) const noexcept;
        // Returns size of wave audio data

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDuration(unsigned int index) const noexcept;
        // Returns the duration in samples

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDurationMS(unsigned int index) const noexcept;
        // Returns the duration in milliseconds

        DIRECTX_TOOLKIT_API const WAVEFORMATEX* __cdecl GetFormat(
            unsigned int index,
            _Out_writes_bytes_(maxsize) WAVEFORMATEX* wfx, size_t maxsize) const noexcept;

        DIRECTX_TOOLKIT_API int __cdecl Find(_In_z_ const char* name) const;

    #ifdef USING_XAUDIO2_9
        DIRECTX_TOOLKIT_API bool __cdecl FillSubmitBuffer(
            unsigned int index,
            _Out_ XAUDIO2_BUFFER& buffer,
            _Out_ XAUDIO2_BUFFER_WMA& wmaBuffer) const;
    #else
        DIRECTX_TOOLKIT_API void __cdecl FillSubmitBuffer(
            unsigned int index,
            _Out_ XAUDIO2_BUFFER& buffer) const;
    #endif

        DIRECTX_TOOLKIT_API void __cdecl UnregisterInstance(_In_ IVoiceNotify* instance);

        DIRECTX_TOOLKIT_API HANDLE __cdecl GetAsyncHandle() const noexcept;

        DIRECTX_TOOLKIT_API bool __cdecl GetPrivateData(
            unsigned int index,
            _Out_writes_bytes_(datasize) void* data, size_t datasize);

    #if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
        DIRECTX_TOOLKIT_API WaveBank(
            _In_ AudioEngine* engine,
            _In_z_ const __wchar_t* wbFileName);
    #endif

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    class SoundEffect
    {
    public:
        DIRECTX_TOOLKIT_API SoundEffect(
            _In_ AudioEngine* engine,
            _In_z_ const wchar_t* waveFileName);

        DIRECTX_TOOLKIT_API SoundEffect(
            _In_ AudioEngine* engine,
            _Inout_ std::unique_ptr<uint8_t[]>& wavData,
            _In_ const WAVEFORMATEX* wfx,
            _In_reads_bytes_(audioBytes) const uint8_t* startAudio, size_t audioBytes);

        DIRECTX_TOOLKIT_API SoundEffect(
            _In_ AudioEngine* engine,
            _Inout_ std::unique_ptr<uint8_t[]>& wavData,
            _In_ const WAVEFORMATEX* wfx,
            _In_reads_bytes_(audioBytes) const uint8_t* startAudio, size_t audioBytes,
            uint32_t loopStart, uint32_t loopLength);

    #ifdef USING_XAUDIO2_9

        DIRECTX_TOOLKIT_API SoundEffect(
            _In_ AudioEngine* engine,
            _Inout_ std::unique_ptr<uint8_t[]>& wavData,
            _In_ const WAVEFORMATEX* wfx,
            _In_reads_bytes_(audioBytes) const uint8_t* startAudio, size_t audioBytes,
            _In_reads_(seekCount) const uint32_t* seekTable, size_t seekCount);

    #endif

        DIRECTX_TOOLKIT_API SoundEffect(SoundEffect&&) noexcept;
        DIRECTX_TOOLKIT_API SoundEffect& operator= (SoundEffect&&) noexcept;

        SoundEffect(SoundEffect const&) = delete;
        SoundEffect& operator= (SoundEffect const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~SoundEffect();

        DIRECTX_TOOLKIT_API void __cdecl Play();
        DIRECTX_TOOLKIT_API void __cdecl Play(float volume, float pitch, float pan);

        DIRECTX_TOOLKIT_API std::unique_ptr<SoundEffectInstance> __cdecl CreateInstance(SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);

        DIRECTX_TOOLKIT_API bool __cdecl IsInUse() const noexcept;

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleSizeInBytes() const noexcept;
        // Returns size of wave audio data

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDuration() const noexcept;
        // Returns the duration in samples

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDurationMS() const noexcept;
        // Returns the duration in milliseconds

        DIRECTX_TOOLKIT_API const WAVEFORMATEX* __cdecl GetFormat() const noexcept;

    #ifdef USING_XAUDIO2_9
        DIRECTX_TOOLKIT_API bool __cdecl FillSubmitBuffer(
            _Out_ XAUDIO2_BUFFER& buffer,
            _Out_ XAUDIO2_BUFFER_WMA& wmaBuffer) const;
    #else
        DIRECTX_TOOLKIT_API void __cdecl FillSubmitBuffer(_Out_ XAUDIO2_BUFFER& buffer) const;
    #endif

        DIRECTX_TOOLKIT_API void __cdecl UnregisterInstance(_In_ IVoiceNotify* instance);

    #if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
        DIRECTX_TOOLKIT_API SoundEffect(
            _In_ AudioEngine* engine,
            _In_z_ const __wchar_t* waveFileName);
    #endif

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    struct DIRECTX_TOOLKIT_API AudioListener : public X3DAUDIO_LISTENER
    {
        X3DAUDIO_CONE   ListenerCone;

        AudioListener() noexcept :
            X3DAUDIO_LISTENER{},
            ListenerCone{}
        {
            OrientFront.z = -1.f;

            OrientTop.y = 1.f;
        }

        void XM_CALLCONV SetPosition(FXMVECTOR v) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Position), v);
        }
        void __cdecl SetPosition(const XMFLOAT3& pos) noexcept
        {
            Position.x = pos.x;
            Position.y = pos.y;
            Position.z = pos.z;
        }

        void XM_CALLCONV SetVelocity(FXMVECTOR v) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Velocity), v);
        }
        void __cdecl SetVelocity(const XMFLOAT3& vel) noexcept
        {
            Velocity.x = vel.x;
            Velocity.y = vel.y;
            Velocity.z = vel.z;
        }

        void XM_CALLCONV SetOrientation(FXMVECTOR forward, FXMVECTOR up) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), forward);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), up);
        }
        void __cdecl SetOrientation(const XMFLOAT3& forward, const XMFLOAT3& up) noexcept
        {
            OrientFront.x = forward.x;  OrientTop.x = up.x;
            OrientFront.y = forward.y;  OrientTop.y = up.y;
            OrientFront.z = forward.z;  OrientTop.z = up.z;
        }

        void XM_CALLCONV SetOrientationFromQuaternion(FXMVECTOR quat) noexcept
        {
            const XMVECTOR forward = XMVector3Rotate(g_XMIdentityR2, quat);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), forward);

            const XMVECTOR up = XMVector3Rotate(g_XMIdentityR1, quat);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), up);
        }

        // Updates velocity and orientation by tracking changes in position over time.
        void XM_CALLCONV Update(FXMVECTOR newPos, XMVECTOR upDir, float dt) noexcept
        {
            if (dt > 0.f)
            {
                const XMVECTOR lastPos = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&Position));

                XMVECTOR vDelta = XMVectorSubtract(newPos, lastPos);
                const XMVECTOR vt = XMVectorReplicate(dt);
                XMVECTOR v = XMVectorDivide(vDelta, vt);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Velocity), v);

                vDelta = XMVector3Normalize(vDelta);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), vDelta);

                v = XMVector3Cross(upDir, vDelta);
                v = XMVector3Normalize(v);

                v = XMVector3Cross(vDelta, v);
                v = XMVector3Normalize(v);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), v);

                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Position), newPos);
            }
        }

        void __cdecl SetOmnidirectional() noexcept
        {
            pCone = nullptr;
        }

        void __cdecl SetCone(const X3DAUDIO_CONE& listenerCone);

        bool __cdecl IsValid() const;
    };


    //----------------------------------------------------------------------------------
    struct DIRECTX_TOOLKIT_API AudioEmitter : public X3DAUDIO_EMITTER
    {
        X3DAUDIO_CONE   EmitterCone;
        float           EmitterAzimuths[XAUDIO2_MAX_AUDIO_CHANNELS];

        AudioEmitter() noexcept :
            X3DAUDIO_EMITTER{},
            EmitterCone{},
            EmitterAzimuths{}
        {
            OrientFront.z = -1.f;

            OrientTop.y =
                ChannelRadius =
                CurveDistanceScaler =
                DopplerScaler = 1.f;

            ChannelCount = 1;
            pChannelAzimuths = EmitterAzimuths;

            InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
        }

        void XM_CALLCONV SetPosition(FXMVECTOR v) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Position), v);
        }
        void __cdecl SetPosition(const XMFLOAT3& pos) noexcept
        {
            Position.x = pos.x;
            Position.y = pos.y;
            Position.z = pos.z;
        }

        void XM_CALLCONV SetVelocity(FXMVECTOR v) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Velocity), v);
        }
        void __cdecl SetVelocity(const XMFLOAT3& vel) noexcept
        {
            Velocity.x = vel.x;
            Velocity.y = vel.y;
            Velocity.z = vel.z;
        }

        void XM_CALLCONV SetOrientation(FXMVECTOR forward, FXMVECTOR up) noexcept
        {
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), forward);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), up);
        }
        void __cdecl SetOrientation(const XMFLOAT3& forward, const XMFLOAT3& up) noexcept
        {
            OrientFront.x = forward.x;  OrientTop.x = up.x;
            OrientFront.y = forward.y;  OrientTop.y = up.y;
            OrientFront.z = forward.z;  OrientTop.z = up.z;
        }

        void XM_CALLCONV SetOrientationFromQuaternion(FXMVECTOR quat) noexcept
        {
            const XMVECTOR forward = XMVector3Rotate(g_XMIdentityR2, quat);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), forward);

            const XMVECTOR up = XMVector3Rotate(g_XMIdentityR1, quat);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), up);
        }

        // Updates velocity and orientation by tracking changes in position over time.
        void XM_CALLCONV Update(FXMVECTOR newPos, XMVECTOR upDir, float dt) noexcept
        {
            if (dt > 0.f)
            {
                const XMVECTOR lastPos = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&Position));

                XMVECTOR vDelta = XMVectorSubtract(newPos, lastPos);
                const XMVECTOR vt = XMVectorReplicate(dt);
                XMVECTOR v = XMVectorDivide(vDelta, vt);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Velocity), v);

                vDelta = XMVector3Normalize(vDelta);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientFront), vDelta);

                v = XMVector3Cross(upDir, vDelta);
                v = XMVector3Normalize(v);

                v = XMVector3Cross(vDelta, v);
                v = XMVector3Normalize(v);
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&OrientTop), v);

                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&Position), newPos);
            }
        }

        void __cdecl SetOmnidirectional() noexcept
        {
            pCone = nullptr;
        }

        // Only used for single-channel emitters.
        void __cdecl SetCone(const X3DAUDIO_CONE& emitterCone);

        // Set multi-channel emitter azimuths based on speaker configuration geometry.
        void __cdecl EnableDefaultMultiChannel(unsigned int channels, float radius = 1.f);

        // Set default volume, LFE, LPF, and reverb curves.
        void __cdecl EnableDefaultCurves() noexcept;
        void __cdecl EnableLinearCurves() noexcept;

        void __cdecl EnableInverseSquareCurves() noexcept
        {
            pVolumeCurve = nullptr;
            pLFECurve = nullptr;
            pLPFDirectCurve = nullptr;
            pLPFReverbCurve = nullptr;
            pReverbCurve = nullptr;
        }

        bool __cdecl IsValid() const;
    };


    //----------------------------------------------------------------------------------
    class SoundEffectInstance
    {
    public:
        DIRECTX_TOOLKIT_API SoundEffectInstance(SoundEffectInstance&&) noexcept;
        DIRECTX_TOOLKIT_API SoundEffectInstance& operator= (SoundEffectInstance&&) noexcept;

        SoundEffectInstance(SoundEffectInstance const&) = delete;
        SoundEffectInstance& operator= (SoundEffectInstance const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~SoundEffectInstance();

        DIRECTX_TOOLKIT_API void __cdecl Play(bool loop = false);
        DIRECTX_TOOLKIT_API void __cdecl Stop(bool immediate = true) noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Pause() noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Resume();

        DIRECTX_TOOLKIT_API void __cdecl SetVolume(float volume);
        DIRECTX_TOOLKIT_API void __cdecl SetPitch(float pitch);
        DIRECTX_TOOLKIT_API void __cdecl SetPan(float pan);

        DIRECTX_TOOLKIT_API void __cdecl Apply3D(
            const X3DAUDIO_LISTENER& listener,
            const X3DAUDIO_EMITTER& emitter, bool rhcoords = true);

        DIRECTX_TOOLKIT_API bool __cdecl IsLooped() const noexcept;

        DIRECTX_TOOLKIT_API SoundState __cdecl GetState() noexcept;

        DIRECTX_TOOLKIT_API unsigned int __cdecl GetChannelCount() const noexcept;

        DIRECTX_TOOLKIT_API IVoiceNotify* __cdecl GetVoiceNotify() const noexcept;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Private constructors
        DIRECTX_TOOLKIT_API SoundEffectInstance(_In_ AudioEngine* engine, _In_ SoundEffect* effect, SOUND_EFFECT_INSTANCE_FLAGS flags);
        DIRECTX_TOOLKIT_API SoundEffectInstance(_In_ AudioEngine* engine, _In_ WaveBank* effect, unsigned int index, SOUND_EFFECT_INSTANCE_FLAGS flags);

        friend std::unique_ptr<SoundEffectInstance> __cdecl SoundEffect::CreateInstance(SOUND_EFFECT_INSTANCE_FLAGS);
        friend std::unique_ptr<SoundEffectInstance> __cdecl WaveBank::CreateInstance(unsigned int, SOUND_EFFECT_INSTANCE_FLAGS);
    };


    //----------------------------------------------------------------------------------
    class SoundStreamInstance
    {
    public:
        DIRECTX_TOOLKIT_API SoundStreamInstance(SoundStreamInstance&&) noexcept;
        DIRECTX_TOOLKIT_API SoundStreamInstance& operator= (SoundStreamInstance&&) noexcept;

        SoundStreamInstance(SoundStreamInstance const&) = delete;
        SoundStreamInstance& operator= (SoundStreamInstance const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~SoundStreamInstance();

        DIRECTX_TOOLKIT_API void __cdecl Play(bool loop = false);
        DIRECTX_TOOLKIT_API void __cdecl Stop(bool immediate = true) noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Pause() noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Resume();

        DIRECTX_TOOLKIT_API void __cdecl SetVolume(float volume);
        DIRECTX_TOOLKIT_API void __cdecl SetPitch(float pitch);
        DIRECTX_TOOLKIT_API void __cdecl SetPan(float pan);

        DIRECTX_TOOLKIT_API void __cdecl Apply3D(
            const X3DAUDIO_LISTENER& listener,
            const X3DAUDIO_EMITTER& emitter, bool rhcoords = true);

        DIRECTX_TOOLKIT_API bool __cdecl IsLooped() const noexcept;

        DIRECTX_TOOLKIT_API SoundState __cdecl GetState() noexcept;

        DIRECTX_TOOLKIT_API unsigned int __cdecl GetChannelCount() const noexcept;

        DIRECTX_TOOLKIT_API IVoiceNotify* __cdecl GetVoiceNotify() const noexcept;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Private constructors
        DIRECTX_TOOLKIT_API SoundStreamInstance(_In_ AudioEngine* engine, _In_ WaveBank* effect, unsigned int index, SOUND_EFFECT_INSTANCE_FLAGS flags);

        friend std::unique_ptr<SoundStreamInstance> __cdecl WaveBank::CreateStreamInstance(unsigned int, SOUND_EFFECT_INSTANCE_FLAGS);
    };


    //----------------------------------------------------------------------------------
    class DynamicSoundEffectInstance
    {
    public:
        DIRECTX_TOOLKIT_API DynamicSoundEffectInstance(
            _In_ AudioEngine* engine,
            _In_ std::function<void __cdecl(DynamicSoundEffectInstance*)> bufferNeeded,
            int sampleRate, int channels, int sampleBits = 16,
            SOUND_EFFECT_INSTANCE_FLAGS flags = SoundEffectInstance_Default);

        DIRECTX_TOOLKIT_API DynamicSoundEffectInstance(DynamicSoundEffectInstance&&) noexcept;
        DIRECTX_TOOLKIT_API DynamicSoundEffectInstance& operator= (DynamicSoundEffectInstance&&) noexcept;

        DynamicSoundEffectInstance(DynamicSoundEffectInstance const&) = delete;
        DynamicSoundEffectInstance& operator= (DynamicSoundEffectInstance const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~DynamicSoundEffectInstance();

        DIRECTX_TOOLKIT_API void __cdecl Play();
        DIRECTX_TOOLKIT_API void __cdecl Stop(bool immediate = true) noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Pause() noexcept;
        DIRECTX_TOOLKIT_API void __cdecl Resume();

        DIRECTX_TOOLKIT_API void __cdecl SetVolume(float volume);
        DIRECTX_TOOLKIT_API void __cdecl SetPitch(float pitch);
        DIRECTX_TOOLKIT_API void __cdecl SetPan(float pan);

        DIRECTX_TOOLKIT_API void __cdecl Apply3D(
            const X3DAUDIO_LISTENER& listener,
            const X3DAUDIO_EMITTER& emitter, bool rhcoords = true);

        DIRECTX_TOOLKIT_API void __cdecl SubmitBuffer(
            _In_reads_bytes_(audioBytes) const uint8_t* pAudioData, size_t audioBytes);
        DIRECTX_TOOLKIT_API void __cdecl SubmitBuffer(
            _In_reads_bytes_(audioBytes) const uint8_t* pAudioData,
            uint32_t offset,
            size_t audioBytes);

        DIRECTX_TOOLKIT_API SoundState __cdecl GetState() noexcept;

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDuration(size_t bytes) const noexcept;
        // Returns duration in samples of a buffer of a given size

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleDurationMS(size_t bytes) const noexcept;
        // Returns duration in milliseconds of a buffer of a given size

        DIRECTX_TOOLKIT_API size_t __cdecl GetSampleSizeInBytes(uint64_t duration) const noexcept;
        // Returns size of a buffer for a duration given in milliseconds

        DIRECTX_TOOLKIT_API int __cdecl GetPendingBufferCount() const noexcept;

        DIRECTX_TOOLKIT_API const WAVEFORMATEX* __cdecl GetFormat() const noexcept;

        DIRECTX_TOOLKIT_API unsigned int __cdecl GetChannelCount() const noexcept;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

    DEFINE_ENUM_FLAG_OPERATORS(AUDIO_ENGINE_FLAGS)
        DEFINE_ENUM_FLAG_OPERATORS(SOUND_EFFECT_INSTANCE_FLAGS)

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
}
