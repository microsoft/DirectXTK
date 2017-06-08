// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929


// HDR10 Media Profile
// https://en.wikipedia.org/wiki/High-dynamic-range_video#HDR10

// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
static const float3x3 from709to2020 =
{
    { 0.6274040f, 0.3292820f, 0.0433136f },
    { 0.0690970f, 0.9195400f, 0.0113612f },
    { 0.0163916f, 0.0880132f, 0.8955950f }
};


// Apply the ST.2084 curve to normalized linear values and outputs normalized non-linear values
float3 LinearToST2084(float3 normalizedLinearValue)
{
    return pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), 0.1593017578f)), 78.84375f);
}


// ST.2084 to linear, resulting in a linear normalized value
float3 ST2084ToLinear(float3 ST2084)
{
    return pow(max(pow(abs(ST2084), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f * pow(abs(ST2084), 1.0f / 78.84375f)), 1.0f / 0.1593017578f);
}


// Reinhard tonemap operator
// Reinhard et al. "Photographic tone reproduction for digital images." ACM Transactions on Graphics. 21. 2002.
// http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 ToneMapReinhard(float3 color)
{
    return color / (1.0f + color);
}


// Filmic tonemap operator
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 ToneMapFilmic(float3 color)
{
    float3 x = max(0.0f, color - 0.004f);
    return pow((x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f), 2.2f);
}

