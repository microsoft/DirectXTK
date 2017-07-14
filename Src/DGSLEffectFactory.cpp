//--------------------------------------------------------------------------------------
// File: DGSLEffectFactory.cpp
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
#include "WICTextureLoader.h"

#include <string.h>

#include "BinaryReader.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
static_assert(DGSLEffect::MaxTextures == DGSLEffectFactory::DGSLEffectInfo::BaseTextureOffset + _countof(DGSLEffectFactory::DGSLEffectInfo::textures), "DGSL supports 8 textures");
#endif

// Internal DGSLEffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing DGSLEffectFactory instances.
class DGSLEffectFactory::Impl
{
public:
    Impl(_In_ ID3D11Device* device)
      : mPath{},
        device(device),
        mSharing(true),
        mForceSRGB(false)
    {}

    std::shared_ptr<IEffect> CreateEffect( _In_ DGSLEffectFactory* factory, _In_ const IEffectFactory::EffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext );
    std::shared_ptr<IEffect> CreateDGSLEffect( _In_ DGSLEffectFactory* factory, _In_ const DGSLEffectInfo& info, _In_opt_ ID3D11DeviceContext* deviceContext );
    void CreateTexture( _In_z_ const wchar_t* texture, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView );
    void CreatePixelShader( _In_z_ const wchar_t* shader, _Outptr_ ID3D11PixelShader** pixelShader );

    void ReleaseCache();
    void SetSharing( bool enabled ) { mSharing = enabled; }
    void EnableForceSRGB(bool forceSRGB) { mForceSRGB = forceSRGB; }

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;

    wchar_t mPath[MAX_PATH];

private:
    ComPtr<ID3D11Device> device;

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;
    typedef std::map< std::wstring, ComPtr<ID3D11ShaderResourceView> > TextureCache;
    typedef std::map< std::wstring, ComPtr<ID3D11PixelShader> > ShaderCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    TextureCache mTextureCache;
    ShaderCache  mShaderCache;

    bool mSharing;
    bool mForceSRGB;

    std::mutex mutex;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, DGSLEffectFactory::Impl> DGSLEffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> DGSLEffectFactory::Impl::CreateEffect( DGSLEffectFactory* factory, const DGSLEffectFactory::EffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    if ( info.enableDualTexture )
    {
        throw std::exception( "DGSLEffect does not support multiple texcoords" );
    }

    if ( mSharing && info.name && *info.name )
    {
        if ( info.enableSkinning )
        {
            auto it = mEffectCacheSkinning.find( info.name );
            if ( it != mEffectCacheSkinning.end() )
            {
                return it->second;
            }
        }
        else
        {
            auto it = mEffectCache.find( info.name );
            if ( it != mEffectCache.end() )
            {
                return it->second;
            }
        }
    }

    auto effect = std::make_shared<DGSLEffect>( device.Get(), nullptr, info.enableSkinning );

    effect->EnableDefaultLighting();
    effect->SetLightingEnabled(true);

    XMVECTOR color = XMLoadFloat3( &info.ambientColor );
    effect->SetAmbientColor( color );

    color = XMLoadFloat3( &info.diffuseColor );
    effect->SetDiffuseColor( color );

    effect->SetAlpha( info.alpha );

    if ( info.perVertexColor )
    {
        effect->SetVertexColorEnabled( true );
    }

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

    if ( info.diffuseTexture && *info.diffuseTexture )
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        factory->CreateTexture( info.diffuseTexture, deviceContext, srv.GetAddressOf() );

        effect->SetTexture( srv.Get() );
        effect->SetTextureEnabled(true);
    }

    if ( mSharing && info.name && *info.name )
    {
        std::lock_guard<std::mutex> lock(mutex);
        if ( info.enableSkinning )
        {
            mEffectCacheSkinning.insert( EffectCache::value_type( info.name, effect ) );
        }
        else
        {
            mEffectCache.insert( EffectCache::value_type( info.name, effect ) );
        }
    }

    return effect;
}


_Use_decl_annotations_
std::shared_ptr<IEffect> DGSLEffectFactory::Impl::CreateDGSLEffect( DGSLEffectFactory* factory, const DGSLEffectFactory::DGSLEffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    if ( mSharing && info.name && *info.name )
    {
        if ( info.enableSkinning )
        {
            auto it = mEffectCacheSkinning.find( info.name );
            if ( it != mEffectCacheSkinning.end() )
            {
                return it->second;
            }
        }
        else
        {
            auto it = mEffectCache.find( info.name );
            if ( it != mEffectCache.end() )
            {
                return it->second;
            }
        }
    }

    std::shared_ptr<DGSLEffect> effect;

    bool lighting = true;
    bool allowSpecular = true;

    if ( !info.pixelShader || !*info.pixelShader )
    {
        effect = std::make_shared<DGSLEffect>( device.Get(), nullptr, info.enableSkinning );
    }
    else
    {
        wchar_t root[ MAX_PATH ] = {};
        auto last = wcsrchr( info.pixelShader, '_' );
        if ( last )
        {
            wcscpy_s( root, last+1 );
        }
        else
        {
            wcscpy_s( root, info.pixelShader );
        }

        auto first = wcschr( root, '.' );
        if ( first )
            *first = 0;

        if ( !_wcsicmp( root, L"lambert" ) )
        {
            allowSpecular = false;
            effect = std::make_shared<DGSLEffect>( device.Get(), nullptr, info.enableSkinning );
        }
        else if ( !_wcsicmp( root, L"phong" ) )
        {
            effect = std::make_shared<DGSLEffect>( device.Get(), nullptr, info.enableSkinning );
        }
        else if ( !_wcsicmp( root, L"unlit" ) )
        {
            lighting = false;
            effect = std::make_shared<DGSLEffect>( device.Get(), nullptr, info.enableSkinning );
        }
        else if ( device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0 )
        {
            // DGSL shaders are not compatible with Feature Level 9.x, use fallback shader
            wcscat_s( root, L".cso" );

            ComPtr<ID3D11PixelShader> ps;
            factory->CreatePixelShader( root, ps.GetAddressOf() );

            effect = std::make_shared<DGSLEffect>( device.Get(), ps.Get(), info.enableSkinning );
        }
        else
        {
            // Create DGSL shader and use it for the effect
            ComPtr<ID3D11PixelShader> ps;
            factory->CreatePixelShader( info.pixelShader, ps.GetAddressOf() );

            effect = std::make_shared<DGSLEffect>( device.Get(), ps.Get(), info.enableSkinning );
        }
    }

    if ( lighting )
    {
        effect->EnableDefaultLighting();
        effect->SetLightingEnabled(true);
    }

    XMVECTOR color = XMLoadFloat3( &info.ambientColor );
    effect->SetAmbientColor( color );

    color = XMLoadFloat3( &info.diffuseColor );
    effect->SetDiffuseColor( color );
    effect->SetAlpha( info.alpha );

    if ( info.perVertexColor )
    {
        effect->SetVertexColorEnabled( true );
    }

    effect->SetAlphaDiscardEnable(true);

    if ( allowSpecular
            && ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 ) )
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

    if ( info.diffuseTexture && *info.diffuseTexture )
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        factory->CreateTexture( info.diffuseTexture, deviceContext, srv.GetAddressOf() );

        effect->SetTexture( srv.Get() );
        effect->SetTextureEnabled(true);
    }

    if ( info.specularTexture && *info.specularTexture )
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        factory->CreateTexture( info.specularTexture, deviceContext, srv.GetAddressOf() );

        effect->SetTexture( 1, srv.Get() );
        effect->SetTextureEnabled(true);
    }

    if ( info.normalTexture && *info.normalTexture )
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        factory->CreateTexture( info.normalTexture, deviceContext, srv.GetAddressOf() );

        effect->SetTexture( 2, srv.Get() );
        effect->SetTextureEnabled(true);
    }

    for( int j = 0; j < _countof(info.textures); ++j )
    {
        if ( info.textures[j] && *info.textures[j] )
        {
            ComPtr<ID3D11ShaderResourceView> srv;

            factory->CreateTexture( info.textures[j], deviceContext, srv.GetAddressOf() );

            effect->SetTexture( j + DGSLEffectInfo::BaseTextureOffset, srv.Get() );
            effect->SetTextureEnabled(true);
        }
    }

    if ( mSharing && info.name && *info.name )
    {
        std::lock_guard<std::mutex> lock(mutex);
        if ( info.enableSkinning )
        {
            mEffectCacheSkinning.insert( EffectCache::value_type( info.name, effect ) );
        }
        else
        {
            mEffectCache.insert( EffectCache::value_type( info.name, effect ) );
        }
    }

    return effect;
}


_Use_decl_annotations_
void DGSLEffectFactory::Impl::CreateTexture( const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView )
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
        wchar_t fullName[MAX_PATH] = {};
        wcscpy_s( fullName, mPath );
        wcscat_s( fullName, name );

        WIN32_FILE_ATTRIBUTE_DATA fileAttr = {};
        if ( !GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr) )
        {
            // Try Current Working Directory (CWD)
            wcscpy_s( fullName, name );
            if ( !GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr) )
            {
                DebugTrace( "DGSLEffectFactory could not find texture file '%ls'\n", name );
                throw std::exception( "CreateTexture" );
            }
        }

        wchar_t ext[_MAX_EXT];
        _wsplitpath_s( name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

        if ( _wcsicmp( ext, L".dds" ) == 0 )
        {
            HRESULT hr = CreateDDSTextureFromFileEx(
                device.Get(), fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB, nullptr, textureView);
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
            HRESULT hr = CreateWICTextureFromFileEx(
                device.Get(), deviceContext, fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT, nullptr, textureView );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateWICTextureFromFile" );
            }
        }
#endif
        else
        {
            HRESULT hr = CreateWICTextureFromFileEx(
                device.Get(), fullName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                mForceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT, nullptr, textureView );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateWICTextureFromFile" );
            }
        }

        if ( mSharing && *name && it == mTextureCache.end() )
        {   
            std::lock_guard<std::mutex> lock(mutex);
            mTextureCache.insert( TextureCache::value_type( name, *textureView ) );
        }
    }
}


_Use_decl_annotations_
void DGSLEffectFactory::Impl::CreatePixelShader( const wchar_t* name, ID3D11PixelShader** pixelShader )
{
    if ( !name || !pixelShader )
        throw std::exception("invalid arguments");

    auto it = mShaderCache.find( name );

    if ( mSharing && it != mShaderCache.end() )
    {
        ID3D11PixelShader* ps = it->second.Get();
        ps->AddRef();
        *pixelShader = ps;
    }
    else
    {
        wchar_t fullName[MAX_PATH] = {};
        wcscpy_s( fullName, mPath );
        wcscat_s( fullName, name );

        WIN32_FILE_ATTRIBUTE_DATA fileAttr = {};
        if ( !GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr) )
        {
            // Try Current Working Directory (CWD)
            wcscpy_s( fullName, name );
            if ( !GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr) )
            {
                DebugTrace( "DGSLEffectFactory could not find shader file '%ls'\n", name );
                throw std::exception( "CreatePixelShader" );
            }
        }

        size_t dataSize = 0;
        std::unique_ptr<uint8_t[]> data;
        HRESULT hr = BinaryReader::ReadEntireFile( fullName, data, &dataSize );
        if ( FAILED(hr) )
        {
            DebugTrace( "CreatePixelShader failed (%08X) to load shader file '%ls'\n", hr, fullName );
            throw std::exception( "CreatePixelShader" );
        }
       
        ThrowIfFailed(
            device->CreatePixelShader( data.get(), dataSize, nullptr, pixelShader ) );

        _Analysis_assume_(*pixelShader != 0);

        if ( mSharing && *name && it == mShaderCache.end() )
        {   
            std::lock_guard<std::mutex> lock(mutex);
            mShaderCache.insert( ShaderCache::value_type( name, *pixelShader ) );
        }
    }
}


void DGSLEffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mTextureCache.clear();
    mShaderCache.clear();
}



//--------------------------------------------------------------------------------------
// DGSLEffectFactory
//--------------------------------------------------------------------------------------

DGSLEffectFactory::DGSLEffectFactory(_In_ ID3D11Device* device)
    : pImpl(Impl::instancePool.DemandCreate(device))
{
}

DGSLEffectFactory::~DGSLEffectFactory()
{
}


DGSLEffectFactory::DGSLEffectFactory(DGSLEffectFactory&& moveFrom)
    : pImpl(std::move(moveFrom.pImpl))
{
}

DGSLEffectFactory& DGSLEffectFactory::operator= (DGSLEffectFactory&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// IEffectFactory methods
_Use_decl_annotations_
std::shared_ptr<IEffect> DGSLEffectFactory::CreateEffect( const EffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    return pImpl->CreateEffect( this, info, deviceContext );
}

_Use_decl_annotations_
void DGSLEffectFactory::CreateTexture( const wchar_t* name, ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** textureView )
{
    return pImpl->CreateTexture( name, deviceContext, textureView );
}


// DGSL methods.
_Use_decl_annotations_
std::shared_ptr<IEffect> DGSLEffectFactory::CreateDGSLEffect( const DGSLEffectInfo& info, ID3D11DeviceContext* deviceContext )
{
    return pImpl->CreateDGSLEffect( this, info, deviceContext );
}


_Use_decl_annotations_
void DGSLEffectFactory::CreatePixelShader( const wchar_t* shader, ID3D11PixelShader** pixelShader )
{
    pImpl->CreatePixelShader( shader, pixelShader );
}


// Settings
void DGSLEffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void DGSLEffectFactory::SetSharing( bool enabled )
{
    pImpl->SetSharing( enabled );
}

void DGSLEffectFactory::EnableForceSRGB(bool forceSRGB)
{
    pImpl->EnableForceSRGB( forceSRGB );
}

void DGSLEffectFactory::SetDirectory( _In_opt_z_ const wchar_t* path )
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