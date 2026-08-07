//--------------------------------------------------------------------------------------
// File: NPREffectFactory.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Effects.h"
#include "DemandCreate.h"
#include "SharedResourcePool.h"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    template<typename T>
    void SetMaterialProperties(_In_ T* effect, const NPREffectFactory::EffectInfo& info, NPREffect::Mode mode)
    {
        effect->SetMode(mode);

        effect->EnableDefaultLighting();

        effect->SetAlpha(info.alpha);

        // Most DirectX Tool Kit effects do not have an ambient material color.

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
        {
            color = XMLoadFloat3(&info.specularColor);
            effect->SetSpecularColor(color);
        }
        else
        {
            effect->DisableSpecular();
        }

        if (info.biasedVertexNormals)
        {
            effect->SetBiasedVertexNormals(true);
        }
    }
}

// Internal NPREffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing NPREffectFactory instances.
class NPREffectFactory::Impl
{
public:
    explicit Impl(_In_ ID3D11Device* device)
        : mPath{},
        mMode(NPREffect::Mode_Cel),
        mDevice(device),
        mSharing(true),
        mForceSRGB(false),
        mUseEmissiveForMatCap(true)
    {
        if (!device)
            throw std::invalid_argument("Direct3D device is null");
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    std::shared_ptr<IEffect> CreateEffect(_In_ NPREffectFactory* factory, _In_ const EffectFactory::EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext);
    void CreateTexture(_In_z_ const wchar_t* texture, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView);

    void ReleaseCache();

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;

    wchar_t mPath[MAX_PATH];

    NPREffect::Mode mMode;

    ComPtr<ID3D11ShaderResourceView> mMatCap;
    ComPtr<ID3D11Device> mDevice;

    bool mSharing;
    bool mForceSRGB;
    bool mUseEmissiveForMatCap;

private:
    using EffectCache = std::map< std::wstring, std::shared_ptr<IEffect> >;
    using TextureCache = std::map< std::wstring, ComPtr<ID3D11ShaderResourceView> >;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;
    TextureCache mTextureCache;

    std::mutex mutex;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, NPREffectFactory::Impl> NPREffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> NPREffectFactory::Impl::CreateEffect(NPREffectFactory* factory, const EffectFactory::EffectInfo& info, ID3D11DeviceContext* deviceContext)
{
    if (info.enableSkinning)
    {
        // SkinnedNPREffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectCacheSkinning.find(info.name);
            if (mSharing && it != mEffectCacheSkinning.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<SkinnedNPREffect>(mDevice.Get());

        SetMaterialProperties(effect.get(), info, mMode);

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;
            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());
            effect->SetTexture(srv.Get());
        }

        if (mMode == NPREffect::Mode_MatCap)
        {
            if (mUseEmissiveForMatCap && info.emissiveTexture && *info.emissiveTexture)
            {
                ComPtr<ID3D11ShaderResourceView> srv;
                factory->CreateTexture(info.emissiveTexture, deviceContext, srv.GetAddressOf());
                effect->SetMatCap(srv.Get());
            }
            else
            {
                effect->SetMatCap(mMatCap.Get());
            }
        }

        if (mSharing && info.name && *info.name)
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(info.name, effect);
            mEffectCacheSkinning.insert(v);
        }

        return std::move(effect);
    }
    else if (info.enableDualTexture)
    {
        // DualTextureEffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectCacheDualTexture.find(info.name);
            if (mSharing && it != mEffectCacheDualTexture.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<DualTextureEffect>(mDevice.Get());

        // Dual texture effect doesn't support lighting (usually it's lightmaps)

        effect->SetAlpha(info.alpha);

        if (info.perVertexColor)
        {
            effect->SetVertexColorEnabled(true);
        }

        const XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture(srv.Get());
        }

        if (info.emissiveTexture && *info.emissiveTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.emissiveTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture2(srv.Get());
        }
        else if (info.specularTexture && *info.specularTexture)
        {
            // If there's no emissive texture specified, use the specular texture as the second texture
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.specularTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture2(srv.Get());
        }

        if (mSharing && info.name && *info.name)
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(info.name, effect);
            mEffectCacheDualTexture.insert(v);
        }

        return std::move(effect);
    }
    else
    {
        // NPREffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectCache.find(info.name);
            if (mSharing && it != mEffectCache.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<NPREffect>(mDevice.Get());

        SetMaterialProperties(effect.get(), info, mMode);

        if (info.perVertexColor)
        {
            effect->SetVertexColorEnabled(true);
        }

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture(srv.Get());
            effect->SetTextureEnabled(true);
        }

        if (mMode == NPREffect::Mode_MatCap)
        {
            // TODO: Use emissiveTexture or specularTexture for matcap?
            effect->SetMatCap(mMatCap.Get());
        }

        if (mSharing && info.name && *info.name)
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(info.name, effect);
            mEffectCache.insert(v);
        }

        return std::move(effect);
    }
}

_Use_decl_annotations_
void NPREffectFactory::Impl::CreateTexture(const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView)
{
    if (!name || !textureView)
        throw std::invalid_argument("name and textureView parameters can't be null");

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(deviceContext);
#endif

    auto it = mTextureCache.find(name);

    if (mSharing && it != mTextureCache.end())
    {
        ID3D11ShaderResourceView* srv = it->second.Get();
        srv->AddRef();
        *textureView = srv;
    }
    else
    {
        wchar_t fullName[MAX_PATH] = {};
        wcscpy_s(fullName, mPath);
        wcscat_s(fullName, name);

        WIN32_FILE_ATTRIBUTE_DATA fileAttr = {};
        if (!GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
        {
            // Try Current Working Directory (CWD)
            wcscpy_s(fullName, name);
            if (!GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
            {
                DebugTrace("ERROR: NPREffectFactory could not find texture file '%ls'\n", name);
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "NPREffectFactory::CreateTexture");
            }
        }

        wchar_t ext[_MAX_EXT] = {};
        _wsplitpath_s(name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);
        const bool isdds = _wcsicmp(ext, L".dds") == 0;

        if (isdds)
        {
            HRESULT hr = CreateDDSTextureFromFileEx(
                mDevice.Get(), fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_DEFAULT, nullptr, textureView);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: CreateDDSTextureFromFile failed (%08X) for '%ls'\n",
                    static_cast<unsigned int>(hr), fullName);
                throw std::runtime_error("NPREffectFactory::CreateDDSTextureFromFile");
            }
        }
    #if !defined(_XBOX_ONE) || !defined(_TITLE)
        else if (deviceContext)
        {
            std::lock_guard<std::mutex> lock(mutex);
            HRESULT hr = CreateWICTextureFromFileEx(
                mDevice.Get(), deviceContext, fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT, nullptr, textureView);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: CreateWICTextureFromFile failed (%08X) for '%ls'\n",
                    static_cast<unsigned int>(hr), fullName);
                throw std::runtime_error("NPREffectFactory::CreateWICTextureFromFile");
            }
        }
    #endif
        else
        {
            HRESULT hr = CreateWICTextureFromFileEx(
                mDevice.Get(), fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT, nullptr, textureView);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: CreateWICTextureFromFile failed (%08X) for '%ls'\n",
                    static_cast<unsigned int>(hr), fullName);
                throw std::runtime_error("NPREffectFactory::CreateWICTextureFromFile");
            }
        }

        if (mSharing && *name && it == mTextureCache.end())
        {
            std::lock_guard<std::mutex> lock(mutex);
            TextureCache::value_type v(name, *textureView);
            mTextureCache.insert(v);
        }
    }
}

void NPREffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
    mTextureCache.clear();
}



//--------------------------------------------------------------------------------------
// NPREffectFactory
//--------------------------------------------------------------------------------------

NPREffectFactory::NPREffectFactory(_In_ ID3D11Device* device)
    : pImpl(Impl::instancePool.DemandCreate(device))
{}


NPREffectFactory::NPREffectFactory(NPREffectFactory&&) noexcept = default;
NPREffectFactory& NPREffectFactory::operator= (NPREffectFactory&&) noexcept = default;
NPREffectFactory::~NPREffectFactory() = default;


_Use_decl_annotations_
std::shared_ptr<IEffect> NPREffectFactory::CreateEffect(const EffectInfo& info, ID3D11DeviceContext* deviceContext)
{
    return pImpl->CreateEffect(this, info, deviceContext);
}

_Use_decl_annotations_
void NPREffectFactory::CreateTexture(const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView)
{
    return pImpl->CreateTexture(name, deviceContext, textureView);
}

void NPREffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void NPREffectFactory::SetSharing(bool enabled) noexcept
{
    pImpl->mSharing = enabled;
}

void NPREffectFactory::EnableForceSRGB(bool forceSRGB) noexcept
{
    pImpl->mForceSRGB = forceSRGB;
}

void NPREffectFactory::SetDirectory(_In_opt_z_ const wchar_t* path) noexcept
{
    if (path && *path != 0)
    {
        wcscpy_s(pImpl->mPath, path);
        size_t len = wcsnlen(pImpl->mPath, MAX_PATH);
        if (len > 0 && len < (MAX_PATH - 1))
        {
            // Ensure it has a trailing slash
            if (pImpl->mPath[len - 1] != L'\\')
            {
                pImpl->mPath[len] = L'\\';
                pImpl->mPath[len + 1] = 0;
            }
        }
    }
    else
        *pImpl->mPath = 0;
}

void NPREffectFactory::SetMode(NPREffect::Mode mode) noexcept
{
    pImpl->mMode = mode;
}

void NPREffectFactory::SetDefaultMatCap(_In_opt_ ID3D11ShaderResourceView* textureView) noexcept
{
    pImpl->mMatCap = textureView;
}


void NPREffectFactory::SetEmissiveAsMatCap(bool value) noexcept
{
    pImpl->mUseEmissiveForMatCap = value;
}


ID3D11Device* NPREffectFactory::GetDevice() const noexcept
{
    return pImpl->mDevice.Get();
}
