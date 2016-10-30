//--------------------------------------------------------------------------------------
// File: Geometry.h
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

#include "VertexTypes.h"

namespace DirectX
{
    typedef std::vector<DirectX::VertexPositionNormalTexture> VertexCollection;
    typedef std::vector<uint16_t> IndexCollection;

    void ComputeBox(VertexCollection& vertices, IndexCollection& indices, const XMFLOAT3& size, bool rhcoords, bool invertn);
    void ComputeSphere(VertexCollection& vertices, IndexCollection& indices, float diameter, size_t tessellation, bool rhcoords, bool invertn);
    void ComputeGeoSphere(VertexCollection& vertices, IndexCollection& indices, float diameter, size_t tessellation, bool rhcoords);
    void ComputeCylinder(VertexCollection& vertices, IndexCollection& indices, float height, float diameter, size_t tessellation, bool rhcoords);
    void ComputeCone(VertexCollection& vertices, IndexCollection& indices, float diameter, float height, size_t tessellation, bool rhcoords);
    void ComputeTorus(VertexCollection& vertices, IndexCollection& indices, float diameter, float thickness, size_t tessellation, bool rhcoords);
    void ComputeTetrahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeOctahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeDodecahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeIcosahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeTeapot(VertexCollection& vertices, IndexCollection& indices, float size, size_t tessellation, bool rhcoords);
}