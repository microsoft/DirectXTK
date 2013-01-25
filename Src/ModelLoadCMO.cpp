//--------------------------------------------------------------------------------------
// File: ModelLoadCMO.cpp
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
#include "Model.h"

#include "DDSTextureLoader.h"
#include "Effects.h"

#include "PlatformHelpers.h"
#include "BinaryReader.h"

using namespace DirectX;
using namespace Microsoft::WRL;

//--------------------------------------------------------------------------------------
// .CMO files are built by Visual Studio 2012 and an example renderer is provided
// in the VS Direct3D Starter Kit
// http://code.msdn.microsoft.com/Visual-Studio-3D-Starter-455a15f1
//--------------------------------------------------------------------------------------

namespace VSD3DStarter
{
    // .CMO files

    // UINT - Mesh count
    // { [Mesh count]
    //      UINT - Length of name
    //      wchar_t[] - Name of mesh (if length > 0)
    //      UINT - Material count
    //      { [Material count]
    //          UINT - Length of material name
    //          wchar_t[] - Name of material (if length > 0)
    //          Material structure
    //          UINT - Length of pixel shader name
    //          wchar_t[] - Name of pixel shader (if length > 0)
    //          { [8]
    //              UINT - Length of texture name
    //              wchar_t[] - Name of texture (if length > 0)
    //          }
    //      }
    //      BYTE - 1 if there is skeletal animation data present
    //      UINT - SubMesh count
    //      { [SubMesh count]
    //          SubMesh structure
    //      }
    //      UINT - IB Count
    //      { [IB Count]
    //          UINT - Number of USHORTs in IB
    //          USHORT[] - Array of indices
    //      }
    //      UINT - VB Count
    //      { [VB Count]
    //          UINT - Number of verts in VB
    //          Vertex[] - Array of vertices
    //      }
    //      UINT - Skinning VB Count
    //      { [Skinning VB Count]
    //          UINT - Number of verts in Skinning VB
    //          SkinningVertex[] - Array of skinning verts
    //      }
    //      MeshExtents structure
    //      [If skeleton animation data is not present, file ends here]
    //      UINT - Bone count
    //      { [Bone count]
    //          UINT - Length of bone name
    //          wchar_t[] - Bone name (if length > 0)
    //          Bone structure
    //      }
    //      UINT - Animation clip count
    //      { [Animation clip count]
    //          UINT - Length of clip name
    //          wchar_t[] - Clip name (if length > 0)
    //          float - Start time
    //          float - End time
    //          UINT - Keyframe count
    //          { [Keyframe count]
    //              Keyframe structure
    //          }
    //      }
    // }

    #pragma pack(push,1)

    struct Material
    {
        DirectX::XMFLOAT4   Ambient;
        DirectX::XMFLOAT4   Diffuse;
        DirectX::XMFLOAT4   Specular;
        float               SpecularPower;
        DirectX::XMFLOAT4   Emissive;
        DirectX::XMFLOAT4X4 UVTransform;
    };

    const uint32_t MAX_TEXTURE = 8;

    struct SubMesh
    {
        UINT MaterialIndex;
        UINT IndexBufferIndex;
        UINT VertexBufferIndex;
        UINT StartIndex;
        UINT PrimCount;
    };

    struct Vertex
    {
        float x, y, z;
        float nx, ny, nz;
        float tx, ty, tz, tw;
        UINT color;
        float u, v;
    };

    const uint32_t NUM_BONE_INFLUENCES = 4;

    struct SkinningVertex
    {
        UINT boneIndex[NUM_BONE_INFLUENCES];
        float boneWeight[NUM_BONE_INFLUENCES];
    };

    struct MeshExtents
    {
        float CenterX, CenterY, CenterZ;
        float Radius;

        float MinX, MinY, MinZ;
        float MaxX, MaxY, MaxZ;
    };

    struct Bone
    {
        INT ParentIndex;
        DirectX::XMFLOAT4X4 InvBindPos;
        DirectX::XMFLOAT4X4 BindPos;
        DirectX::XMFLOAT4X4 LocalTransform;
    };
    
    struct Clip
    {
        float StartTime;
        float EndTime;
        UINT  keys;
    };

    struct Keyframe
    {
        UINT BoneIndex;
        float Time;
        DirectX::XMFLOAT4X4 Transform;
    };

    #pragma pack(pop)

}; // namespace

//--------------------------------------------------------------------------------------
struct MaterialRecordCMO
{
    const VSD3DStarter::Material*   pMaterial;
    std::wstring                    name;
    std::wstring                    pixelShader;
    std::wstring                    texture[VSD3DStarter::MAX_TEXTURE];
    std::shared_ptr<IEffect>        effect;
    ComPtr<ID3D11InputLayout>       il;
};

static const D3D11_INPUT_ELEMENT_DESC g_InputElements[] =
{
    { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",       0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Helper for creating a D3D input layout.
static void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
{
    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    ThrowIfFailed(
        device->CreateInputLayout(g_InputElements,
                                  _countof(g_InputElements),
                                  shaderByteCode, byteCodeLength,
                                  pInputLayout)
    );

    SetDebugObjectName(*pInputLayout, "ModelCMO");
}

// Shared VB input element description
static INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;
static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> g_vbdecl;

static BOOL CALLBACK InitializeDecl( PINIT_ONCE initOnce, PVOID Parameter, PVOID *lpContext )
{
    UNREFERENCED_PARAMETER( initOnce );
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( lpContext );
    g_vbdecl = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>>( g_InputElements, g_InputElements + _countof(g_InputElements) );
    return TRUE;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromCMO( ID3D11Device* d3dDevice, const uint8_t* meshData, size_t dataSize, IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    if ( !InitOnceExecuteOnce( &g_InitOnce, InitializeDecl, nullptr, nullptr ) )
        throw std::exception("One-time initialization failed");
    
    if ( !d3dDevice || !meshData )
        throw std::exception("Device and meshData cannot be null");

    // Meshes
    auto nMesh = reinterpret_cast<const UINT*>( meshData );
    size_t usedSize = sizeof(UINT);
    if ( dataSize < usedSize )
        throw std::exception("End of file");

    if ( !*nMesh )
        throw std::exception("No meshes found");

    std::unique_ptr<Model> model(new Model());

    for( UINT meshIndex = 0; meshIndex < *nMesh; ++meshIndex )
    {
        // Mesh name
        auto nName = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        auto meshName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

        usedSize += sizeof(wchar_t)*(*nName);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        auto mesh = std::make_shared<ModelMesh>();
        mesh->name.assign( meshName, *nName );
        mesh->ccw = ccw;
        mesh->pmalpha = pmalpha;

        // Materials
        auto nMats = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        std::vector<MaterialRecordCMO> materials;
        materials.reserve( *nMats );
        for( UINT j = 0; j < *nMats; ++j )
        {
            MaterialRecordCMO m;

            // Material name
            nName = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            auto matName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

            usedSize += sizeof(wchar_t)*(*nName);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            m.name.assign( matName, *nName );

            // Material settings
            auto matSetting = reinterpret_cast<const VSD3DStarter::Material*>( meshData + usedSize );
            usedSize += sizeof(VSD3DStarter::Material);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            m.pMaterial = matSetting;

            // Pixel shader name
            nName = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            auto psName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

            usedSize += sizeof(wchar_t)*(*nName);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            m.pixelShader.assign( psName, *nName );

            for( UINT t = 0; t < VSD3DStarter::MAX_TEXTURE; ++t )
            {
                nName = reinterpret_cast<const UINT*>( meshData + usedSize );
                usedSize += sizeof(UINT);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");

                auto txtName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

                usedSize += sizeof(wchar_t)*(*nName);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");

                m.texture[t].assign( txtName, *nName );
            }

            EffectFactory::EffectInfo info;
            info.name = m.name.c_str();
            info.specularPower = m.pMaterial->SpecularPower;
            info.perVertexColor = true;
            info.alpha = m.pMaterial->Diffuse.w;
            info.ambientColor = XMFLOAT3( m.pMaterial->Ambient.x, m.pMaterial->Ambient.y, m.pMaterial->Ambient.z );
            info.diffuseColor = XMFLOAT3( m.pMaterial->Diffuse.x, m.pMaterial->Diffuse.y, m.pMaterial->Diffuse.z );
            info.specularColor = XMFLOAT3( m.pMaterial->Specular.x, m.pMaterial->Specular.y, m.pMaterial->Specular.z );
            info.emissiveColor = XMFLOAT3( m.pMaterial->Emissive.x, m.pMaterial->Emissive.y, m.pMaterial->Emissive.z );
            info.texture = m.texture[0].c_str();

            m.effect = fxFactory.CreateEffect( info, nullptr );

            CreateInputLayout( d3dDevice, m.effect.get(), &m.il );

            materials.emplace_back( m );
        }

        // Skeletal data?
        auto bSkeleton = reinterpret_cast<const BYTE*>( meshData + usedSize );
        usedSize += sizeof(BYTE);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        // Submeshes
        auto nSubmesh = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        if ( !*nSubmesh )
            throw std::exception("No submeshes found\n");

        auto subMesh = reinterpret_cast<const VSD3DStarter::SubMesh*>( meshData + usedSize );
        usedSize += sizeof(VSD3DStarter::SubMesh) * (*nSubmesh);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        // Index buffers
        auto nIBs = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        if ( !*nIBs )
            throw std::exception("No index buffers found\n");

        std::vector<ComPtr<ID3D11Buffer>> ibs;
        ibs.resize( *nIBs );
        for( UINT j = 0; j < *nIBs; ++j )
        {
            auto nIndexes = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            if ( !*nIndexes )
                throw std::exception("Empty index buffer found\n");

            size_t ibBytes = sizeof(USHORT) * (*(nIndexes));

            auto indexes = reinterpret_cast<const USHORT*>( meshData + usedSize );
            usedSize += ibBytes;
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            D3D11_BUFFER_DESC desc = {0};
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.ByteWidth = static_cast<UINT>( ibBytes );
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {0};
            initData.pSysMem = indexes;

            ThrowIfFailed(
                d3dDevice->CreateBuffer( &desc, &initData, &ibs[j] )
                );

            SetDebugObjectName( ibs[j].Get(), "ModelCMO" ); 
        }

        // Vertex buffers
        auto nVBs = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        if ( !*nVBs )
            throw std::exception("No vertex buffers found\n");

        std::vector<ComPtr<ID3D11Buffer>> vbs;
        vbs.resize( *nVBs );
        for( UINT j = 0; j < *nVBs; ++j )
        {
            auto nVerts = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            if ( !*nVerts )
                throw std::exception("Empty vertex buffer found\n");

            size_t vbBytes = sizeof(VSD3DStarter::Vertex) * (*(nVerts));

            auto verts = reinterpret_cast<const VSD3DStarter::Vertex*>( meshData + usedSize );
            usedSize += vbBytes;
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            D3D11_BUFFER_DESC desc = {0};
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.ByteWidth = static_cast<UINT>( vbBytes );
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {0};
            initData.pSysMem = verts;

            ThrowIfFailed(
                d3dDevice->CreateBuffer( &desc, &initData, &vbs[j] )
                );

            SetDebugObjectName( vbs[j].Get(), "ModelCMO" ); 
        }

        // Skinning vertex buffers
        auto nSkinVBs = reinterpret_cast<const UINT*>( meshData + usedSize );
        usedSize += sizeof(UINT);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        for( UINT j = 0; j < *nSkinVBs; ++j )
        {
            auto nVerts = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            if ( !*nVerts )
                throw std::exception("Empty skinning vertex buffer found\n");

            size_t vbBytes = sizeof(VSD3DStarter::SkinningVertex) * (*(nVerts));

            auto verts = reinterpret_cast<const VSD3DStarter::SkinningVertex*>( meshData + usedSize );
            usedSize += vbBytes;
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            // TODO - What to do with skinning data?
            verts;
        }

        // Extents
        auto extents = reinterpret_cast<const VSD3DStarter::MeshExtents*>( meshData + usedSize );
        usedSize += sizeof(VSD3DStarter::MeshExtents);
        if ( dataSize < usedSize )
            throw std::exception("End of file");

        mesh->boundingSphere.Center.x = extents->CenterX;
        mesh->boundingSphere.Center.y = extents->CenterY;
        mesh->boundingSphere.Center.z = extents->CenterZ;
        mesh->boundingSphere.Radius = extents->Radius;

        XMVECTOR min = XMVectorSet( extents->MinX, extents->MinY, extents->MinZ, 0.f );
        XMVECTOR max = XMVectorSet( extents->MaxX, extents->MaxY, extents->MaxZ, 0.f );
        BoundingBox::CreateFromPoints( mesh->boundingBox, min, max );

#if 0
        // Animation data
        if ( *bSkeleton )
        {
            // Bones
            auto nBones = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            if ( !*nBones )
                throw std::exception("Animation bone data is missing\n");

            for( UINT j = 0; j < *nBones; ++j )
            {
                // Bone name
                nName = reinterpret_cast<const UINT*>( meshData + usedSize );
                usedSize += sizeof(UINT);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");

                auto boneName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

                usedSize += sizeof(wchar_t)*(*nName);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");
                
                // TODO - What to do with bone name?
                boneName;

                // Bone settings
                auto bones = reinterpret_cast<const VSD3DStarter::Bone*>( meshData + usedSize );
                usedSize += sizeof(VSD3DStarter::Bone);
                if ( dataSize < usedSize )  
                    throw std::exception("End of file");

                // TODO - What to do with bone data?
                bones;
            }

            // Animation Clips
            auto nClips = reinterpret_cast<const UINT*>( meshData + usedSize );
            usedSize += sizeof(UINT);
            if ( dataSize < usedSize )
                throw std::exception("End of file");

            for( UINT j = 0; j < *nClips; ++j )
            {
                // Clip name
                nName = reinterpret_cast<const UINT*>( meshData + usedSize );
                usedSize += sizeof(UINT);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");

                auto clipName = reinterpret_cast<const wchar_t*>( meshData + usedSize );

                usedSize += sizeof(wchar_t)*(*nName);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");
                
                // TODO - What to do with clip name?
                clipName;

                auto clip = reinterpret_cast<const VSD3DStarter::Clip*>( meshData + usedSize );
                usedSize += sizeof(VSD3DStarter::Clip);
                if ( dataSize < usedSize )
                    throw std::exception("End of file");

                if ( !clip->keys )
                    throw std::exception("Keyframes missing in clip");

                auto keys = reinterpret_cast<const VSD3DStarter::Keyframe*>( meshData + usedSize );
                usedSize += sizeof(VSD3DStarter::Keyframe) * clip->keys;
                if ( dataSize < usedSize )  
                    throw std::exception("End of file");

                // TODO - What to do with keys and clip->StartTime, clip->EndTime?
                keys;
            }
        }
#else
        bSkeleton;
#endif

        // Build mesh parts
        for( UINT j = 0; j < *nSubmesh; ++j )
        {
            auto& sm = subMesh[j];

            if ( (sm.IndexBufferIndex >= *nIBs)
                 || (sm.VertexBufferIndex >= *nVBs)
                 || (sm.MaterialIndex >= *nMats) )
                 throw std::exception("Invalid submesh found\n");

            auto& mat = materials[ sm.MaterialIndex ];

            auto part = new ModelMeshPart();

            if ( mat.pMaterial->Diffuse.w < 1 )
                part->isAlpha = true;

            part->indexCount = sm.PrimCount * 3;
            part->startIndex = sm.StartIndex;
            part->vertexStride = sizeof(VSD3DStarter::Vertex);
            part->inputLayout = mat.il;
            part->indexBuffer = ibs[ sm.IndexBufferIndex ];
            part->vertexBuffer = vbs[ sm.VertexBufferIndex ];
            part->effect = mat.effect;
            part->vbDecl = g_vbdecl;

            mesh->meshParts.emplace_back( part );
        }

        model->meshes.emplace_back( mesh );
    }

    return model;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromCMO( ID3D11Device* d3dDevice, const wchar_t* szFileName, IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    size_t dataSize = 0;
    std::unique_ptr<uint8_t[]> data;
    ThrowIfFailed(
        BinaryReader::ReadEntireFile( szFileName, data, &dataSize )
    );

    auto model = CreateFromCMO( d3dDevice, data.get(), dataSize, fxFactory, ccw, pmalpha );

    model->name = szFileName;

    return model;
}
