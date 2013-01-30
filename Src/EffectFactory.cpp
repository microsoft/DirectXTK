//--------------------------------------------------------------------------------------
// File: EffectFactory.cpp
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
#include "Effects.h"
#include "DemandCreate.h"
#include "SharedResourcePool.h"

#include "DDSTextureLoader.h"

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
#include "WICTextureLoader.h"
#endif

using namespace DirectX;
using namespace Microsoft::WRL;


// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D11Device* device)
      : device(device), mSharing(true)
    { }

    std::shared_ptr<IEffect> CreateEffect( _In_ IEffectFactory* factory, _In_ const IEffectFactory::EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext );
    void CreateTexture( _In_z_ const WCHAR* texture, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView );

    void ReleaseCache();
    void SetSharing( bool enabled ) { mSharing = enabled; }

    ComPtr<ID3D11Device> device;

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;
    typedef std::map< std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > TextureCache;

    EffectCache  mEffectCache;
    TextureCache mTextureCache;

    bool mSharing;

    std::mutex mutex;

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, EffectFactory::Impl> EffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect( IEffectFactory* factory, const IEffectFactory::EffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    auto it = (info.name && *info.name) ? mEffectCache.find( info.name ) : mEffectCache.end();

    if ( mSharing && it != mEffectCache.end() )
    {
        return it->second;
    }
    else
    {
        std::shared_ptr<BasicEffect> effect = std::make_shared<BasicEffect>( device.Get() );

        effect->EnableDefaultLighting();
        effect->SetLightingEnabled(true);

        effect->SetAlpha( info.alpha );

        if ( info.perVertexColor )
        {
            effect->SetVertexColorEnabled( true );
        }

        XMVECTOR color = XMLoadFloat3( &info.ambientColor );
        effect->SetAmbientLightColor( color );

        color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 )
        {
            color = XMLoadFloat3( &info.specularColor );
            effect->SetSpecularColor( color );
            effect->SetSpecularPower( info.specularPower );
        }

        if ( info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0 )
        {
            color = XMLoadFloat3( &info.emissiveColor );
            effect->SetEmissiveColor( color );
        }

        if ( info.texture && *info.texture )
        {
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture( info.texture, deviceContext, &srv );

            effect->SetTexture( srv.Get() );
            effect->SetTextureEnabled( true );
        }

        if ( mSharing && info.name && *info.name && it == mEffectCache.end() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCache.insert( EffectCache::value_type( info.name, effect) );
        }

        return effect;
    }
}

_Use_decl_annotations_
void EffectFactory::Impl::CreateTexture( const WCHAR* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView )
{
    if ( !name || !textureView )
        throw std::exception("invalid arguments");

    auto it = mTextureCache.find( name );

    if ( mSharing && it != mTextureCache.end() )
    {
        ID3D11ShaderResourceView* srv = it->second.Get();
        srv->AddRef();
        *textureView = srv;
    }
    else
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
        WCHAR ext[_MAX_EXT];
        _wsplitpath_s( name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

        if ( _wcsicmp( ext, L".dds" ) == 0 )
        {
            ThrowIfFailed(
                CreateDDSTextureFromFile( device.Get(), name, nullptr, textureView )
                );
        }
        else if ( deviceContext )
        {
            std::lock_guard<std::mutex> lock(mutex);
            DirectX::ThrowIfFailed(
                CreateWICTextureFromFile( device.Get(), deviceContext, name, nullptr, textureView )
                );
        }
        else
        {
            DirectX::ThrowIfFailed(
                CreateWICTextureFromFile( device.Get(), nullptr, name, nullptr, textureView )
                );
        }
#else
        UNREFERENCED_PARAMETER( deviceContext );
        ThrowIfFailed(
            CreateDDSTextureFromFile( device.Get(), name, nullptr, textureView ) );
#endif

        if ( mSharing && *name && it == mTextureCache.end() )
        {   
            std::lock_guard<std::mutex> lock(mutex);
            mTextureCache.insert( TextureCache::value_type( name, *textureView ) );
        }
    }
}

void EffectFactory::Impl::ReleaseCache()
{
    mEffectCache.clear();
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


EffectFactory::EffectFactory(EffectFactory&& moveFrom)
    : pImpl(std::move(moveFrom.pImpl))
{
}

EffectFactory& EffectFactory::operator= (EffectFactory&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}

_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::CreateEffect( const EffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    return pImpl->CreateEffect( this, info, deviceContext );
}

_Use_decl_annotations_
void EffectFactory::CreateTexture( const WCHAR* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView )
{
    return pImpl->CreateTexture( name, deviceContext, textureView );
}

void EffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void EffectFactory::SetSharing( bool enabled )
{
    pImpl->SetSharing( enabled );
}
