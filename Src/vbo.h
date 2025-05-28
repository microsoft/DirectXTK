//--------------------------------------------------------------------------------------
// File: vbo.h
//
// The VBO file format was introduced in the Windows 8.0 ResourceLoading sample. It's
// a simple binary file containing a 16-bit index buffer and a fixed-format vertex buffer.
//
// The meshconvert sample tool for DirectXMesh can produce this file type
// http://go.microsoft.com/fwlink/?LinkID=324981
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>

namespace VBO
{
#pragma pack(push,1)

    struct header_t
    {
        uint32_t numVertices;
        uint32_t numIndices;
    };

    struct vertex_t
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 textureCoordinate;
    };

#pragma pack(pop)

} // namespace

static_assert(sizeof(VBO::header_t) == 8, "VBO header size mismatch");
static_assert(sizeof(VBO::vertex_t) == 32, "VBO vertex size mismatch");
