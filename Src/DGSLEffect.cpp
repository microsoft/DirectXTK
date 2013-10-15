//--------------------------------------------------------------------------------------
// File: DGSLEffect.cpp
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
#include "EffectCommon.h"
#include "DemandCreate.h"

//
// Based on the Visual Studio 3D Starter Kit
//
// http://aka.ms/vs3dkit
//

namespace DirectX
{

namespace EffectDirtyFlags
{
    const int ConstantBufferMaterial    = 0x10000;
    const int ConstantBufferLight       = 0x20000;
    const int ConstantBufferObject      = 0x40000;
    const int ConstantBufferMisc        = 0x80000;
}

}


using namespace DirectX;

// Constant buffer layout. Must match the shader!
#pragma pack(push,1)

// Slot 0
struct MaterialConstants
{
    XMVECTOR    Ambient;
    XMVECTOR    Diffuse;
    XMVECTOR    Specular;
    XMVECTOR    Emissive;
    float       SpecularPower;
    float       Padding0;
    float       Padding1;
    float       Padding2;
};

// Slot 1
const int DGSLMaxLights = 4;

struct LightConstants
{
    XMVECTOR    Ambient;
    XMVECTOR    LightColor[DGSLMaxLights];
    XMVECTOR    LightAttenuation[DGSLMaxLights];
    XMVECTOR    LightDirection[DGSLMaxLights];
    XMVECTOR    LightSpecularIntensity[DGSLMaxLights];
    UINT        IsPointLight[DGSLMaxLights];
    UINT        ActiveLights;
    float       Padding0;
    float       Padding1;
    float       Padding2;
};

// Note - DGSL does not appear to make use of LightAttenuation or IsPointLight. Not sure if it uses ActiveLights either.

// Slot 2
struct ObjectConstants
{
    XMMATRIX    LocalToWorld4x4;
    XMMATRIX    LocalToProjected4x4;
    XMMATRIX    WorldToLocal4x4;
    XMMATRIX    WorldToView4x4;
    XMMATRIX    UvTransform4x4;
    XMVECTOR    EyePosition;
};

// Slot 3
struct MiscConstants
{
    float ViewportWidth;
    float ViewportHeight;
    float Time;
    float Padding1;
};

#pragma pack(pop)

static_assert( ( sizeof(MaterialConstants) % 16 ) == 0, "CB size not padded correctly" );
static_assert( ( sizeof(LightConstants) % 16 ) == 0, "CB size not padded correctly" );
static_assert( ( sizeof(ObjectConstants) % 16 ) == 0, "CB size not padded correctly" );
static_assert( ( sizeof(MiscConstants) % 16 ) == 0, "CB size not padded correctly" );

__declspec(align(16)) struct DGSLEffectConstants
{
    MaterialConstants   material;
    LightConstants      light;
    ObjectConstants     object;
    MiscConstants       misc;
};


// Include the precompiled shader code.
namespace
{
    #include "Shaders/Compiled/DGSLEffect_main.inc"

    #include "Shaders/Compiled/DGSLUnlit_main.inc"
    #include "Shaders/Compiled/DGSLLambert_main.inc"
    #include "Shaders/Compiled/DGSLPhong_main.inc"

    #include "Shaders/Compiled/DGSLUnlit_mainTx.inc"
    #include "Shaders/Compiled/DGSLLambert_mainTx.inc"
    #include "Shaders/Compiled/DGSLPhong_mainTx.inc"
}


class DGSLEffect::Impl : public AlignedNew<DGSLEffectConstants>
{
public:
    Impl( _In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader ) :
        dirtyFlags( INT_MAX ),
        textureEnabled(false),
        specularEnabled(false),
        mPixelShader( pixelShader ),
        mCBMaterial( device ),
        mCBLight( device ),
        mCBObject( device ),
        mCBMisc( device ),
        mDeviceResources( deviceResourcesPool.DemandCreate(device) )
    {
        memset( &constants, 0, sizeof(constants) );

        XMMATRIX id = XMMatrixIdentity();
        world = id;
        view = id;
        projection = id;
        constants.object.UvTransform4x4 = id;
    }

    // Methods
    void Apply( _In_ ID3D11DeviceContext* deviceContext );

    // Fields
    DGSLEffectConstants constants;

    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textures[MaxTextures];

    int dirtyFlags;

    bool textureEnabled;
    bool specularEnabled;

private:
    ConstantBuffer<MaterialConstants>           mCBMaterial;
    ConstantBuffer<LightConstants>              mCBLight;
    ConstantBuffer<ObjectConstants>             mCBObject;
    ConstantBuffer<MiscConstants>               mCBMisc;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   mPixelShader;

    // Only one of these helpers is allocated per D3D device, even if there are multiple effect instances.
    class DeviceResources : protected EffectDeviceResources
    {
    public:
        DeviceResources(_In_ ID3D11Device* device) : EffectDeviceResources(device) {}

        // Gets or lazily creates the vertex shader.
        ID3D11VertexShader* GetVertexShader()
        {
            static const ShaderBytecode s_shader = { DGSLEffect_main, sizeof(DGSLEffect_main) };

            return DemandCreateVertexShader(mVertexShader, s_shader);
        }

        // Gets or lazily creates the specified pixel shader permutation.
        ID3D11PixelShader* GetPixelShader( bool textureEnabled, bool specularEnabled, bool lightingEnabled )
        {
            static const ShaderBytecode s_shaders[MaxPixelShaders] =
            {
                { DGSLUnlit_main, sizeof(DGSLUnlit_main) },         // UNLIT (no texture)
                { DGSLLambert_main, sizeof(DGSLLambert_main) },     // LAMBERT (no texture)
                { DGSLPhong_main, sizeof(DGSLPhong_main) },         // PHONG (no texture)

                { DGSLUnlit_mainTx, sizeof(DGSLUnlit_mainTx) },     // UNLIT (textured)
                { DGSLLambert_mainTx, sizeof(DGSLLambert_mainTx) }, // LAMBERT (textured)
                { DGSLPhong_mainTx, sizeof(DGSLPhong_mainTx) },     // PHONG (textured)
            };

            int defaultShader = 0;

            if ( lightingEnabled )
            {
                defaultShader = ( specularEnabled ) ? 2 : 1;
            }

            if ( textureEnabled )
                defaultShader += 3;

            if ( defaultShader >= _countof(s_shaders) )
                throw std::out_of_range("defaultShader parameter out of range");

            return DemandCreatePixelShader(mPixelShaders[defaultShader], s_shaders[defaultShader]);
        }

        ID3D11ShaderResourceView* GetDefaultTexture()
        {
            return DemandCreate(mDefaultTexture, mMutex, [&](ID3D11ShaderResourceView** pResult) -> HRESULT
            {
                static const uint32_t s_pixel = 0xffffffff;
                
                D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

                D3D11_TEXTURE2D_DESC desc;
                memset( &desc, 0, sizeof(desc) );
                desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                ID3D11Texture2D* tex = nullptr;
                HRESULT hr = mDevice->CreateTexture2D( &desc, &initData, &tex );

                if (SUCCEEDED(hr))
                {
                    SetDebugObjectName(tex, "DirectXTK:DGSLEffect");

                    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    SRVDesc.Texture2D.MipLevels = 1;

                    hr = mDevice->CreateShaderResourceView( tex, &SRVDesc, pResult );
                    if (SUCCEEDED(hr))
                        SetDebugObjectName(*pResult, "DirectXTK:DGSLEffect");
                    else
                        tex->Release();
                }

                return hr;
            });
        }

    private:
        static const int MaxPixelShaders = 6;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShaders[MaxPixelShaders];
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mDefaultTexture;
    };

    // Per-device resources.
    std::shared_ptr<DeviceResources> mDeviceResources;

    static SharedResourcePool<ID3D11Device*, DeviceResources> deviceResourcesPool;
};


SharedResourcePool<ID3D11Device*, DGSLEffect::Impl::DeviceResources> DGSLEffect::Impl::deviceResourcesPool;


void DGSLEffect::Impl::Apply( _In_ ID3D11DeviceContext* deviceContext )
{
    auto vertexShader = mDeviceResources->GetVertexShader();
    auto pixelShader = mPixelShader.Get();
    if( !pixelShader )
    {
        pixelShader = mDeviceResources->GetPixelShader( textureEnabled, specularEnabled, constants.light.ActiveLights > 0 );
    }

    deviceContext->VSSetShader( vertexShader, nullptr, 0 );
    deviceContext->PSSetShader( pixelShader, nullptr, 0 );

    // Check for any required matrices updates
    if (dirtyFlags & EffectDirtyFlags::WorldViewProj)
    {
        constants.object.LocalToWorld4x4 = XMMatrixTranspose( world );
        constants.object.WorldToView4x4 = XMMatrixTranspose( view );

        XMMATRIX worldView = XMMatrixMultiply( world, view );

        constants.object.LocalToProjected4x4 = XMMatrixTranspose( XMMatrixMultiply( worldView, projection ) );
                
        dirtyFlags &= ~EffectDirtyFlags::WorldViewProj;
        dirtyFlags |= EffectDirtyFlags::ConstantBufferObject;
    }

    if (dirtyFlags & EffectDirtyFlags::WorldInverseTranspose)
    {
        XMMATRIX worldInverse = XMMatrixInverse( nullptr, world );

        constants.object.WorldToLocal4x4 = XMMatrixTranspose( worldInverse );

        dirtyFlags &= ~EffectDirtyFlags::WorldInverseTranspose;
        dirtyFlags |= EffectDirtyFlags::ConstantBufferObject;
    }

    if (dirtyFlags & EffectDirtyFlags::EyePosition)
    {
        XMMATRIX viewInverse = XMMatrixInverse( nullptr, view );
        
        constants.object.EyePosition = viewInverse.r[3];

        dirtyFlags &= ~EffectDirtyFlags::EyePosition;
        dirtyFlags |= EffectDirtyFlags::ConstantBufferObject;
    }

    // Make sure the constant buffers are up to date.
    if (dirtyFlags & EffectDirtyFlags::ConstantBufferMaterial)
    {
        mCBMaterial.SetData(deviceContext, constants.material);
     
        dirtyFlags &= ~EffectDirtyFlags::ConstantBufferMaterial;
    }

    if (dirtyFlags & EffectDirtyFlags::ConstantBufferLight)
    {
        mCBLight.SetData(deviceContext, constants.light);
     
        dirtyFlags &= ~EffectDirtyFlags::ConstantBufferLight;
    }

    if (dirtyFlags & EffectDirtyFlags::ConstantBufferObject)
    {
        mCBObject.SetData(deviceContext, constants.object);
     
        dirtyFlags &= ~EffectDirtyFlags::ConstantBufferObject;
    }

    if (dirtyFlags & EffectDirtyFlags::ConstantBufferMisc)
    {
        mCBMisc.SetData(deviceContext, constants.misc);
     
        dirtyFlags &= ~EffectDirtyFlags::ConstantBufferMisc;
    }

    // Set the constant buffer.
    ID3D11Buffer* buffers[4] = { mCBMaterial.GetBuffer(), mCBLight.GetBuffer(), mCBObject.GetBuffer(), mCBMisc.GetBuffer() };

    deviceContext->VSSetConstantBuffers( 0, 4, buffers );
    deviceContext->PSSetConstantBuffers( 0, 4, buffers );

    // Set the textures
    if ( textureEnabled )
    {
        ID3D11ShaderResourceView* txt[MaxTextures] = { textures[0].Get(), textures[1].Get(), textures[2].Get(), textures[3].Get(),
                                                       textures[4].Get(), textures[5].Get(), textures[6].Get(), textures[7].Get() };
        deviceContext->PSSetShaderResources( 0, MaxTextures, txt );
    }
    else
    {
        ID3D11ShaderResourceView* txt[MaxTextures] = { mDeviceResources->GetDefaultTexture(), 0 };
        deviceContext->PSSetShaderResources( 0, MaxTextures, txt );
    }
}


//--------------------------------------------------------------------------------------
// DGSLEffect
//--------------------------------------------------------------------------------------

DGSLEffect::DGSLEffect(_In_ ID3D11Device* device, _In_opt_ ID3D11PixelShader* pixelShader)
    : pImpl(new Impl(device, pixelShader))
{
}


DGSLEffect::DGSLEffect(DGSLEffect&& moveFrom)
    : pImpl(std::move(moveFrom.pImpl))
{
}


DGSLEffect& DGSLEffect::operator= (DGSLEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


DGSLEffect::~DGSLEffect()
{
}


// IEffect methods
void DGSLEffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    pImpl->Apply(deviceContext);
}


void DGSLEffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
    // DGSL always uses the same Vertex Shader
    *pShaderByteCode = DGSLEffect_main;
    *pByteCodeLength = sizeof( DGSLEffect_main );
}


// Camera settings
void XM_CALLCONV DGSLEffect::SetWorld(FXMMATRIX value)
{
    pImpl->world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose;
}


void XM_CALLCONV DGSLEffect::SetView(FXMMATRIX value)
{
    pImpl->view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition;
}


void XM_CALLCONV DGSLEffect::SetProjection(FXMMATRIX value)
{
    pImpl->projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


// Material settings
void XM_CALLCONV DGSLEffect::SetAmbientColor(FXMVECTOR value)
{
    pImpl->constants.material.Ambient = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void XM_CALLCONV DGSLEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->constants.material.Diffuse = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void XM_CALLCONV DGSLEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->constants.material.Emissive = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void XM_CALLCONV DGSLEffect::SetSpecularColor(FXMVECTOR value)
{
    pImpl->specularEnabled = true;
    pImpl->constants.material.Specular = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void DGSLEffect::SetSpecularPower(float value)
{
    pImpl->specularEnabled = true;
    pImpl->constants.material.SpecularPower = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void DGSLEffect::DisableSpecular()
{
    pImpl->specularEnabled = false;
    pImpl->constants.material.Specular = g_XMZero;
    pImpl->constants.material.SpecularPower = 1.f;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


void DGSLEffect::SetAlpha(float value)
{
    // Set w to new value, but preserve existing xyz (diffuse color).
    pImpl->constants.material.Diffuse = XMVectorSetW(pImpl->constants.material.Diffuse, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMaterial;
}


// Additional settings.
void DGSLEffect::SetUVTransform(FXMMATRIX value)
{
    pImpl->constants.object.UvTransform4x4 = XMMatrixTranspose( value );

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferObject;
}


void DGSLEffect::SetViewport( float width, float height )
{
    pImpl->constants.misc.ViewportWidth = width;
    pImpl->constants.misc.ViewportHeight = height;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMisc;
}


void DGSLEffect::SetTime( float time )
{
    pImpl->constants.misc.Time = time;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferMisc;
}


// Light settings
void DGSLEffect::SetLightingEnabled(bool value)
{
    if (value)
    {
        if ( !pImpl->constants.light.ActiveLights )
            pImpl->constants.light.ActiveLights = 1;
    }
    else
    {
        pImpl->constants.light.ActiveLights = 0;
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void DGSLEffect::SetPerPixelLighting(bool value)
{
    UNREFERENCED_PARAMETER(value);
    // DGSL shaders never implement vertex lighting
}


void XM_CALLCONV DGSLEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->constants.light.Ambient = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void DGSLEffect::SetLightEnabled(int whichLight, bool value)
{
    if ( whichLight < 0 || whichLight >= DGSLMaxLights )
        throw std::out_of_range("whichLight parameter out of range");

    if ( value )
    {
        if ( whichLight >= (int)pImpl->constants.light.ActiveLights )
            pImpl->constants.light.ActiveLights = static_cast<UINT>( whichLight + 1 );
    }
    else
    {
        // Only way to disable individual lights with DGSL is to set the colors to black
        pImpl->constants.light.LightColor[whichLight] = g_XMZero;
        pImpl->constants.light.LightSpecularIntensity[whichLight] = g_XMZero;
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void XM_CALLCONV DGSLEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    if ( whichLight < 0 || whichLight >= DGSLMaxLights )
        throw std::out_of_range("whichLight parameter out of range");

    // DGSL effects lights do not negate the direction like BasicEffect
    pImpl->constants.light.LightDirection[whichLight] = XMVectorNegate( value );

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void XM_CALLCONV DGSLEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    if ( whichLight < 0 || whichLight >= DGSLMaxLights )
        throw std::out_of_range("whichLight parameter out of range");

    pImpl->constants.light.LightColor[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void XM_CALLCONV DGSLEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    if ( whichLight < 0 || whichLight >= DGSLMaxLights )
        throw std::out_of_range("whichLight parameter out of range");

    pImpl->constants.light.LightSpecularIntensity[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferLight;
}


void DGSLEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Texture settings
void DGSLEffect::SetTextureEnabled(bool value)
{
    pImpl->textureEnabled = value;
}

void DGSLEffect::SetTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->textures[0] = value;
}

void DGSLEffect::SetTexture(int whichTexture, _In_opt_ ID3D11ShaderResourceView* value)
{
    if ( whichTexture < 0 || whichTexture >= MaxTextures )
        throw std::out_of_range("whichTexture parameter out of range");

    pImpl->textures[ whichTexture ] = value;
}
