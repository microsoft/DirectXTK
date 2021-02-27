//--------------------------------------------------------------------------------------
// File: Model.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#include <dxgiformat.h>
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <wrl\client.h>

#include <DirectXMath.h>
#include <DirectXCollision.h>


namespace DirectX
{
    class IEffect;
    class IEffectFactory;
    class CommonStates;
    class ModelMesh;

    //----------------------------------------------------------------------------------
    // Model loading options
    enum ModelLoaderFlags : uint32_t
    {
        ModelLoader_Clockwise           = 0x0,
        ModelLoader_CounterClockwise    = 0x1,
        ModelLoader_PremultipledAlpha   = 0x2,
        ModelLoader_MaterialColorsSRGB  = 0x4,
        ModelLoader_AllowLargeModels    = 0x8,
    };

    //----------------------------------------------------------------------------------
    // Each mesh part is a submesh with a single effect
    class ModelMeshPart
    {
    public:
        ModelMeshPart() noexcept;

        ModelMeshPart(ModelMeshPart&&) = default;
        ModelMeshPart& operator= (ModelMeshPart&&) = default;

        ModelMeshPart(ModelMeshPart const&) = default;
        ModelMeshPart& operator= (ModelMeshPart const&) = default;

        virtual ~ModelMeshPart();

        uint32_t                                                indexCount;
        uint32_t                                                startIndex;
        int32_t                                                 vertexOffset;
        uint32_t                                                vertexStride;
        D3D_PRIMITIVE_TOPOLOGY                                  primitiveType;
        DXGI_FORMAT                                             indexFormat;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>               inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                    indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                    vertexBuffer;
        std::shared_ptr<IEffect>                                effect;
        std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>  vbDecl;
        bool                                                    isAlpha;

        using Collection = std::vector<std::unique_ptr<ModelMeshPart>>;

        // Draw mesh part with custom effect
        void __cdecl Draw(
            _In_ ID3D11DeviceContext* deviceContext,
            _In_ IEffect* ieffect,
            _In_ ID3D11InputLayout* iinputLayout,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) const;

        void __cdecl DrawInstanced(
            _In_ ID3D11DeviceContext* deviceContext,
            _In_ IEffect* ieffect,
            _In_ ID3D11InputLayout* iinputLayout,
            uint32_t instanceCount,
            uint32_t startInstanceLocation = 0,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) const;

       // Create input layout for drawing with a custom effect.
        void __cdecl CreateInputLayout(_In_ ID3D11Device* device, _In_ IEffect* ieffect, _Outptr_ ID3D11InputLayout** iinputLayout) const;

        // Change effect used by part and regenerate input layout (be sure to call Model::Modified as well)
        void __cdecl ModifyEffect(_In_ ID3D11Device* device, _In_ std::shared_ptr<IEffect>& ieffect, bool isalpha = false);
    };


    //----------------------------------------------------------------------------------
    // A mesh consists of one or more model mesh parts
    class ModelMesh
    {
    public:
        ModelMesh() noexcept;

        ModelMesh(ModelMesh&&) = default;
        ModelMesh& operator= (ModelMesh&&) = default;

        ModelMesh(ModelMesh const&) = default;
        ModelMesh& operator= (ModelMesh const&) = default;

        virtual ~ModelMesh();

        BoundingSphere              boundingSphere;
        BoundingBox                 boundingBox;
        ModelMeshPart::Collection   meshParts;
        std::wstring                name;
        bool                        ccw;
        bool                        pmalpha;

        using Collection = std::vector<std::shared_ptr<ModelMesh>>;

        // Setup states for drawing mesh
        void __cdecl PrepareForRendering(_In_ ID3D11DeviceContext* deviceContext, const CommonStates& states, bool alpha = false, bool wireframe = false) const;

        // Draw the mesh
        void XM_CALLCONV Draw(
            _In_ ID3D11DeviceContext* deviceContext,
            FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection,
            bool alpha = false,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) const;
    };


    //----------------------------------------------------------------------------------
    // A model consists of one or more meshes
    class Model
    {
    public:
        Model() = default;

        Model(Model&&) = default;
        Model& operator= (Model&&) = default;

        Model(Model const&) = default;
        Model& operator= (Model const&) = default;

        virtual ~Model();

        ModelMesh::Collection   meshes;
        std::wstring            name;

        // Draw all the meshes in the model
        void XM_CALLCONV Draw(
            _In_ ID3D11DeviceContext* deviceContext,
            const CommonStates& states,
            FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection,
            bool wireframe = false,
            _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) const;

        // Notify model that effects, parts list, or mesh list has changed
        void __cdecl Modified() noexcept { mEffectCache.clear(); }

        // Update all effects used by the model
        void __cdecl UpdateEffects(_In_ std::function<void __cdecl(IEffect*)> setEffect);

        // Loads a model from a Visual Studio Starter Kit .CMO file
        static std::unique_ptr<Model> __cdecl CreateFromCMO(
            _In_ ID3D11Device* device,
            _In_reads_bytes_(dataSize) const uint8_t* meshData, size_t dataSize,
            _In_ IEffectFactory& fxFactory,
            ModelLoaderFlags flags = ModelLoader_CounterClockwise);
        static std::unique_ptr<Model> __cdecl CreateFromCMO(
            _In_ ID3D11Device* device,
            _In_z_ const wchar_t* szFileName,
            _In_ IEffectFactory& fxFactory,
            ModelLoaderFlags flags = ModelLoader_CounterClockwise);

        // Loads a model from a DirectX SDK .SDKMESH file
        static std::unique_ptr<Model> __cdecl CreateFromSDKMESH(
            _In_ ID3D11Device* device,
            _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
            _In_ IEffectFactory& fxFactory,
            ModelLoaderFlags flags = ModelLoader_Clockwise);
        static std::unique_ptr<Model> __cdecl CreateFromSDKMESH(
            _In_ ID3D11Device* device,
            _In_z_ const wchar_t* szFileName,
            _In_ IEffectFactory& fxFactory,
            ModelLoaderFlags flags = ModelLoader_Clockwise);

        // Loads a model from a .VBO file
        static std::unique_ptr<Model> __cdecl CreateFromVBO(
            _In_ ID3D11Device* device,
            _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
            _In_opt_ std::shared_ptr<IEffect> ieffect = nullptr,
            ModelLoaderFlags flags = ModelLoader_Clockwise);
        static std::unique_ptr<Model> __cdecl CreateFromVBO(
            _In_ ID3D11Device* device,
            _In_z_ const wchar_t* szFileName,
            _In_opt_ std::shared_ptr<IEffect> ieffect = nullptr,
            ModelLoaderFlags flags = ModelLoader_Clockwise);

    private:
        std::set<IEffect*>  mEffectCache;
    };

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

    DEFINE_ENUM_FLAG_OPERATORS(ModelLoaderFlags);

#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
