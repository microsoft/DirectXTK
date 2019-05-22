//--------------------------------------------------------------------------------------
// File: EffectFactory.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Effects.h"
#include "DemandCreate.h"
#include "SharedResourcePool.h"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D11Device* device)
        : mPath{},
        mDevice(device),
        mSharing(true),
        mUseNormalMapEffect(true),
        mForceSRGB(false)
    {}

    std::shared_ptr<IEffect> CreateEffect(_In_ IEffectFactory* factory, _In_ const IEffectFactory::EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext);
    void CreateTexture(_In_z_ const wchar_t* texture, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView);

    void ReleaseCache();
    void SetSharing(bool enabled) { mSharing = enabled; }
    void EnableNormalMapEffect(bool enabled) { mUseNormalMapEffect = enabled; }
    void EnableForceSRGB(bool forceSRGB) { mForceSRGB = forceSRGB; }

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;

    wchar_t mPath[MAX_PATH];

    ComPtr<ID3D11Device> mDevice;

private:
    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;
    typedef std::map< std::wstring, ComPtr<ID3D11ShaderResourceView> > TextureCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;
    EffectCache  mEffectNormalMap;
    TextureCache mTextureCache;

    bool mSharing;
    bool mUseNormalMapEffect;
    bool mForceSRGB;

    std::mutex mutex;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, EffectFactory::Impl> EffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect(IEffectFactory* factory, const IEffectFactory::EffectInfo& info, ID3D11DeviceContext* deviceContext)
{
    if (info.enableSkinning)
    {
        // SkinnedEffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectCacheSkinning.find(info.name);
            if (mSharing && it != mEffectCacheSkinning.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<SkinnedEffect>(mDevice.Get());

        effect->EnableDefaultLighting();

        effect->SetAlpha(info.alpha);

        // Skinned Effect does not have an ambient material color, or per-vertex color support

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
        {
            color = XMLoadFloat3(&info.specularColor);
            effect->SetSpecularColor(color);
            effect->SetSpecularPower(info.specularPower);
        }
        else
        {
            effect->DisableSpecular();
        }

        if (info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0)
        {
            color = XMLoadFloat3(&info.emissiveColor);
            effect->SetEmissiveColor(color);
        }

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture(srv.Get());
        }

        if (info.biasedVertexNormals)
        {
            effect->SetBiasedVertexNormals(true);
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

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
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
    else if (info.enableNormalMaps && mUseNormalMapEffect)
    {
        // NormalMapEffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectNormalMap.find(info.name);
            if (mSharing && it != mEffectNormalMap.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<NormalMapEffect>(mDevice.Get());

        effect->EnableDefaultLighting();

        effect->SetAlpha(info.alpha);

        if (info.perVertexColor)
        {
            effect->SetVertexColorEnabled(true);
        }

        // NormalMap Effect does not have an ambient material color

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
        {
            color = XMLoadFloat3(&info.specularColor);
            effect->SetSpecularColor(color);
            effect->SetSpecularPower(info.specularPower);
        }
        else
        {
            effect->DisableSpecular();
        }

        if (info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0)
        {
            color = XMLoadFloat3(&info.emissiveColor);
            effect->SetEmissiveColor(color);
        }

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture(srv.Get());
        }

        if (info.specularTexture && *info.specularTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.specularTexture, deviceContext, srv.GetAddressOf());

            effect->SetSpecularTexture(srv.Get());
        }

        if (info.normalTexture && *info.normalTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.normalTexture, deviceContext, srv.GetAddressOf());

            effect->SetNormalTexture(srv.Get());
        }

        if (info.biasedVertexNormals)
        {
            effect->SetBiasedVertexNormals(true);
        }

        if (mSharing && info.name && *info.name)
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(info.name, effect);
            mEffectNormalMap.insert(v);
        }

        return std::move(effect);
    }
    else
    {
        // BasicEffect
        if (mSharing && info.name && *info.name)
        {
            auto it = mEffectCache.find(info.name);
            if (mSharing && it != mEffectCache.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<BasicEffect>(mDevice.Get());

        effect->EnableDefaultLighting();
        effect->SetLightingEnabled(true);

        effect->SetAlpha(info.alpha);

        if (info.perVertexColor)
        {
            effect->SetVertexColorEnabled(true);
        }

        // Basic Effect does not have an ambient material color

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
        {
            color = XMLoadFloat3(&info.specularColor);
            effect->SetSpecularColor(color);
            effect->SetSpecularPower(info.specularPower);
        }
        else
        {
            effect->DisableSpecular();
        }

        if (info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0)
        {
            color = XMLoadFloat3(&info.emissiveColor);
            effect->SetEmissiveColor(color);
        }

        if (info.diffuseTexture && *info.diffuseTexture)
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture(info.diffuseTexture, deviceContext, srv.GetAddressOf());

            effect->SetTexture(srv.Get());
            effect->SetTextureEnabled(true);
        }

        if (info.biasedVertexNormals)
        {
            effect->SetBiasedVertexNormals(true);
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
void EffectFactory::Impl::CreateTexture(const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView)
{
    if (!name || !textureView)
        throw std::exception("invalid arguments");

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
                DebugTrace("ERROR: EffectFactory could not find texture file '%ls'\n", name);
                throw std::exception("CreateTexture");
            }
        }

        wchar_t ext[_MAX_EXT];
        _wsplitpath_s(name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

        if (_wcsicmp(ext, L".dds") == 0)
        {
            HRESULT hr = CreateDDSTextureFromFileEx(
                mDevice.Get(), fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB, nullptr, textureView);
            if (FAILED(hr))
            {
                DebugTrace("ERROR: CreateDDSTextureFromFile failed (%08X) for '%ls'\n", hr, fullName);
                throw std::exception("CreateDDSTextureFromFile");
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
                DebugTrace("ERROR: CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName);
                throw std::exception("CreateWICTextureFromFile");
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
                DebugTrace("ERROR: CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName);
                throw std::exception("CreateWICTextureFromFile");
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

void EffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
    mEffectNormalMap.clear();
    mTextureCache.clear();
}



//--------------------------------------------------------------------------------------
// EffectFactory
//--------------------------------------------------------------------------------------

EffectFactory::EffectFactory(_In_ ID3D11Device* device)
    : pImpl(Impl::instancePool.DemandCreate(device))
{
}

EffectFactory::~EffectFactory()
{
}


EffectFactory::EffectFactory(EffectFactory&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
}

EffectFactory& EffectFactory::operator= (EffectFactory&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}

_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::CreateEffect(const EffectInfo& info, ID3D11DeviceContext* deviceContext)
{
    return pImpl->CreateEffect(this, info, deviceContext);
}

_Use_decl_annotations_
void EffectFactory::CreateTexture(const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView)
{
    return pImpl->CreateTexture(name, deviceContext, textureView);
}

void EffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void EffectFactory::SetSharing(bool enabled)
{
    pImpl->SetSharing(enabled);
}

void EffectFactory::EnableNormalMapEffect(bool enabled)
{
    pImpl->EnableNormalMapEffect(enabled);
}

void EffectFactory::EnableForceSRGB(bool forceSRGB)
{
    pImpl->EnableForceSRGB(forceSRGB);
}

void EffectFactory::SetDirectory(_In_opt_z_ const wchar_t* path)
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

ID3D11Device* EffectFactory::GetDevice() const
{
    return pImpl->mDevice.Get();
}
