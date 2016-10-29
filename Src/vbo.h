//--------------------------------------------------------------------------------------
// File: vbo.h
//
// The VBO file format was introduced in the Windows 8.0 ResourceLoading sample. It's
// a simple binary file containing a 16-bit index buffer and a fixed-format vertex buffer.
//
// The meshconvert sample tool for DirectXMesh can produce this file type
// http://go.microsoft.com/fwlink/?LinkID=324981
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

#pragma once


namespace VBO
{
#pragma pack(push,1)

    struct header_t
    {
        uint32_t numVertices;
        uint32_t numIndices;
    };

#pragma pack(pop)

}; // namespace

static_assert(sizeof(VBO::header_t) == 8, "VBO header size mismatch");

