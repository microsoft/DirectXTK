//--------------------------------------------------------------------------------------
// File: ModelLoadGLTF.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Model.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "VertexTypes.h"
#include "BinaryReader.h"
#include "PlatformHelpers.h"

#include "nlohmann/json.hpp"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
#pragma pack(push,1)

    struct glbHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t length;
    };

    struct glbChunk
    {
        uint32_t chunkLength;
        uint32_t chunkType;
    };

#pragma pack(pop)

    constexpr size_t MIN_VALID_GLTF_JSON_LENGTH = 28;
        // minimal valid glTF json is '{"asset":{"version": "2.0"}}'

    void ParseJSON(
        const uint8_t* meshData,
        size_t dataSize,
        std::vector<const uint8_t*>& buffers,
        std::vector<std::unique_ptr<uint8_t[]>>& workingBuffers)
    {
        using json = nlohmann::json;

        try
        {
            auto meta = json::parse(meshData, meshData + dataSize);

            auto version = meta[ "asset" ][ "version" ];
            std::string value;
            version.get_to(value);
            if (strcmp(value.c_str(), "2.0") != 0)
                throw std::runtime_error("Loader only supports glTF 2.0");

            // TODO -
        }
        catch (json::exception&)
        {
            throw std::runtime_error("Failed parsing .gltf");
        }
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> Model::CreateFromGLTF(
    ID3D11Device* device,
    const uint8_t* meshData, size_t dataSize,
    IEffectFactory& fxFactory,
    ModelLoaderFlags flags)
{
    if (!device || !meshData)
        throw std::invalid_argument("Device and meshData cannot be null");

    if (dataSize < MIN_VALID_GLTF_JSON_LENGTH)
        throw std::runtime_error("Invalid .gltf/.glb file");

    const uint8_t* jsonData = nullptr;
    size_t jsonSize = 0;
    std::vector<const uint8_t*> buffers;
    std::vector<std::unique_ptr<uint8_t[]>> workingBuffers;

    const auto header = reinterpret_cast<const glbHeader*>(meshData);
    if (header->magic != 0x46546C67) // "glTF"
    {
        // .gltf JSON file
        jsonData = meshData;
        jsonSize = dataSize;
    }
    else if (header->version != 2)
    {
        throw std::runtime_error("Not a supported .glb file");
    }
    else if (header->length > dataSize)
    {
        throw std::runtime_error("Not enough data for .glb file");
    }
    else
    {
        // .glb binary file
        const uint64_t maxLen = std::min<uint64_t>(header->length, dataSize);
        uint64_t curLength = sizeof(glbHeader);
        const uint8_t* ptr = meshData + sizeof(glbHeader);

        while ((curLength + sizeof(glbChunk)) < maxLen)
        {
            const auto chunk = reinterpret_cast<const glbChunk*>(ptr);
            curLength += sizeof(glbChunk);
            ptr += sizeof(glbChunk);

            if (static_cast<uint64_t>(chunk->chunkLength) + curLength >= maxLen)
            {
                throw std::runtime_error("Invalid chunk in .glb file");
            }

            switch (chunk->chunkType)
            {
            case 0x4E4F534A: // gltf
                if (chunk->chunkLength < MIN_VALID_GLTF_JSON_LENGTH)
                {
                    throw std::runtime_error("Invalid gltf chunk in .glb file");
                }
                if (jsonData)
                {
                    throw std::runtime_error("Invalid .glb file");
                }
                jsonData = ptr;
                jsonSize = chunk->chunkLength;
                break;

            case 0x004E4942: // bin
                if (!chunk->chunkLength)
                {
                    throw std::runtime_error("Invalid bin chunk in .glb file");
                }
                buffers.emplace_back(ptr);
                break;

            default:
                // Skip unknown chunks
                break;
            }

            curLength += chunk->chunkLength;
            ptr += chunk->chunkLength;
        }

        if (!jsonData)
        {
            throw std::runtime_error("Invalid .glb file");
        }
    }

    ParseJSON(jsonData, jsonSize, buffers, workingBuffers);

    // TODO -

    throw std::runtime_error("E_NOIMPL");
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> Model::CreateFromGLTF(
    ID3D11Device* device,
    const wchar_t* szFileName,
    IEffectFactory& fxFactory,
    ModelLoaderFlags flags)
{
    // TODO: For .GLB, it would be more memory efficient to only load the known chunks in case there are extension chunks

    size_t dataSize = 0;
    std::unique_ptr<uint8_t[]> data;
    HRESULT hr = BinaryReader::ReadEntireFile(szFileName, data, &dataSize);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: CreateFromGLTF failed (%08X) loading '%ls'\n",
            static_cast<unsigned int>(hr), szFileName);
        throw std::runtime_error("CreateFromGLTF");
    }

    auto model = CreateFromGLTF(device, data.get(), dataSize, fxFactory, flags);

    model->name = szFileName;

    return model;
}


//--------------------------------------------------------------------------------------
// Adapters for /Zc:wchar_t- clients

#if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)

_Use_decl_annotations_
std::unique_ptr<Model> Model::CreateFromGLTF(
    ID3D11Device* device,
    const __wchar_t* szFileName,
    std::shared_ptr<IEffect> ieffect,
    ModelLoaderFlags flags)
{
    return CreateFromGLTF(device, reinterpret_cast<const unsigned short*>(szFileName), ieffect, flags);
}

#endif
