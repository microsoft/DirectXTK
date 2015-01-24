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

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP) || (_WIN32_WINNT > _WIN32_WINNT_WIN8)
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
    { *mPath = 0; }

    std::shared_ptr<IEffect> CreateEffect( _In_ IEffectFactory* factory, _In_ const IEffectFactory::EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext );
    void CreateTexture( _In_z_ const WCHAR* texture, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView );

    void ReleaseCache();
    void SetSharing( bool enabled ) { mSharing = enabled; }

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;

    WCHAR mPath[MAX_PATH];

private:
    ComPtr<ID3D11Device> device;

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;
    typedef std::map< std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > TextureCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    TextureCache mTextureCache;

    bool mSharing;

    std::mutex mutex;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, EffectFactory::Impl> EffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect( IEffectFactory* factory, const IEffectFactory::EffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    if ( info.enableSkinning )
    {
        // SkinnedEffect
        if ( mSharing && info.name && *info.name )
        {
            auto it = mEffectCacheSkinning.find( info.name );
            if ( mSharing && it != mEffectCacheSkinning.end() )
            {
                return it->second;
            }
        }

        std::shared_ptr<SkinnedEffect> effect = std::make_shared<SkinnedEffect>( device.Get() );

        effect->EnableDefaultLighting();

        effect->SetAlpha( info.alpha );

        // Skinned Effect does not have an ambient material color, or per-vertex color support

        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 )
        {
            color = XMLoadFloat3( &info.specularColor );
            effect->SetSpecularColor( color );
            effect->SetSpecularPower( info.specularPower );
        }
        else
        {
            effect->DisableSpecular();
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
        }

        if ( mSharing && info.name && *info.name )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheSkinning.insert( EffectCache::value_type( info.name, effect) );
        }

        return effect;
    }
    else
    {
        // BasicEffect
        if ( mSharing && info.name && *info.name )
        {
            auto it = mEffectCache.find( info.name );
            if ( mSharing && it != mEffectCache.end() )
            {
                return it->second;
            }
        }

        std::shared_ptr<BasicEffect> effect = std::make_shared<BasicEffect>( device.Get() );

        effect->EnableDefaultLighting();
        effect->SetLightingEnabled(true);

        effect->SetAlpha( info.alpha );

        if ( info.perVertexColor )
        {
            effect->SetVertexColorEnabled( true );
        }

        // Basic Effect does not have an ambient material color

        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 )
        {
            color = XMLoadFloat3( &info.specularColor );
            effect->SetSpecularColor( color );
            effect->SetSpecularPower( info.specularPower );
        }
        else
        {
            effect->DisableSpecular();
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

        if ( mSharing && info.name && *info.name )
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(deviceContext);
#endif

    auto it = mTextureCache.find( name );

    if ( mSharing && it != mTextureCache.end() )
    {
        ID3D11ShaderResourceView* srv = it->second.Get();
        srv->AddRef();
        *textureView = srv;
    }
    else
    {
        WCHAR fullName[MAX_PATH]={0};
        wcscpy_s( fullName, mPath );
        wcscat_s( fullName, name );

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP) || (_WIN32_WINNT > _WIN32_WINNT_WIN8)
        WCHAR ext[_MAX_EXT];
        _wsplitpath_s( name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

        if ( _wcsicmp( ext, L".dds" ) == 0 )
        {
            HRESULT hr = CreateDDSTextureFromFile( device.Get(), fullName, nullptr, textureView );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateDDSTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateDDSTextureFromFile" );
            }
        }
#if !defined(_XBOX_ONE) || !defined(_TITLE)
        else if ( deviceContext )
        {
            std::lock_guard<std::mutex> lock(mutex);
            HRESULT hr = CreateWICTextureFromFile( device.Get(), deviceContext, fullName, nullptr, textureView );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateWICTextureFromFile" );
            }
        }
#endif
        else
        {
            HRESULT hr = CreateWICTextureFromFile( device.Get(), fullName, nullptr, textureView );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateWICTextureFromFile" );
            }
        }
#else
        UNREFERENCED_PARAMETER( deviceContext );
        HRESULT hr = CreateDDSTextureFromFile( device.Get(), fullName, nullptr, textureView );
        if ( FAILED(hr) )
        {
            DebugTrace( "CreateDDSTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
            throw std::exception( "CreateDDSTextureFromFile" );
        }
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
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
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

void EffectFactory::SetDirectory( _In_opt_z_ const WCHAR* path )
{
    if ( path && *path != 0 )
    {
        wcscpy_s( pImpl->mPath, path );
        size_t len = wcsnlen( pImpl->mPath, MAX_PATH );
        if ( len > 0 && len < (MAX_PATH-1) )
        {
            // Ensure it has a trailing slash
            if ( pImpl->mPath[len-1] != L'\\' )
            {
                pImpl->mPath[len] = L'\\';
                pImpl->mPath[len+1] = 0;
            }
        }
    }
    else
        *pImpl->mPath = 0;
}