// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://create.msdn.com/en-US/education/catalog/sample/stock_effects

struct CommonVSOutputPixelLighting
{
    float4 Pos_ps;
    float3 Pos_ws;
    float3 Normal_ws;
};

struct VSOut_Velocity
{
    VSOutputPixelLightingTxTangent current;
    float4                         prevPosition : TEXCOORD4;
};

CommonVSOutputPixelLighting ComputeCommonVSOutputPixelLighting(float4 position, float3 normal)
{
    CommonVSOutputPixelLighting vout;

    vout.Pos_ps = mul(position, WorldViewProj);
    vout.Pos_ws = mul(position, World).xyz;
    vout.Normal_ws = normalize(mul(normal, WorldInverseTranspose));

    return vout;
}

static const float PI = 3.14159265f;

// Shlick's approximation of Fresnel
float3 Fresnel_Shlick(in float3 f0, in float3 f90, in float x)
{
    return f0 + (f90 - f0) * pow(1.f - x, 5.f);
}

// No frills Lambert shading.
float Diffuse_Lambert(in float NdotL)
{
    return NdotL;
}

// Burley's diffuse BRDF
float Diffuse_Burley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2.f * roughness * LdotH * LdotH;
    return Fresnel_Shlick(1, fd90, NdotL).x * Fresnel_Shlick(1, fd90, NdotV).x;
}

// GGX specular D (normal distribution)
float Specular_D_GGX(in float alpha, in float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1)) + 1;
    const float result = alpha2 / (PI * lower * lower);

    return result;
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
float G_Shlick_Smith_Hable(float alpha, float LdotH)
{
    const float k = alpha / 2.0;
    const float k2 = k * k;
    const float invk2 = 1 - k2;
    return rcp(LdotH * LdotH * invk2 + k2);
}

// Map a normal on unit sphere to UV coordinates
float2 SphereMap(float3 N)
{
    return float2(atan2(N.x, N.z) / (PI * 2) + 0.5, 0.5 - (asin(N.y) / PI));
}

// A microfacet based BRDF.
//
// alpha:           This is roughness * roughness as in the "Disney" PBR model by Burley et al.
//
// specularColor:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals. This follows model 
//                  used by Unreal Engine 4.
//
// NdotV, NdotL, LdotH, NdotH: vector relationships between,
//      N - surface normal
//      V - eye normal
//      L - light normal
//      H - half vector between L & V.
float3 Specular_BRDF(in float alpha, in float3 specularColor, in float NdotV, in float NdotL, in float LdotH, in float NdotH)
{
    // Specular D (microfacet normal distribution) component
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // Specular Fresnel
    float3 specular_F = Fresnel_Shlick(specularColor, 1, LdotH);

    // Specular G (visibility) component
    float specular_G = G_Shlick_Smith_Hable(alpha, LdotH);

    return specular_D * specular_F * specular_G;
}

// Diffuse irradiance
float3 Diffuse_IBL(in float3 N)
{
    return IrradianceTexture.Sample(IBLSampler, N);
}

// Approximate specular image based lighting by sampling radiance map at lower mips 
// according to roughness, then modulating by Fresnel term. 
float3 Specular_IBL(in float3 N, in float3 V, in float lodBias)
{
    float mip = lodBias * NumRadianceMipLevels;
    float3 dir = reflect(-V, N);
    float3 envColor = RadianceTexture.SampleLevel(IBLSampler, dir, mip);
    return envColor;
}

// Apply Disney-style physically based rendering to a surface with:
//
// V, N:             Eye and surface normals
//
// numLights:        Number of directional lights.
//
// lightColor[]:     Color and intensity of directional light.
//
// lightDirection[]: Light direction.
float3 LightSurface(
    in float3 V, in float3 N,
    in int numLights, in float3 lightColor[3], in float3 lightDirection[3],
    in float3 albedo, in float roughness, in float metallic, in float ambientOcclusion)
{
    // Specular coefficiant - fixed reflectance value for non-metals
    static const float kSpecularCoefficient = 0.04;

    const float NdotV = saturate(dot(N, V));

    // Burley roughness bias
    const float alpha = roughness * roughness;    

    // Blend base colors
    const float3 c_diff = lerp(albedo, float3(0, 0, 0), metallic)       * ambientOcclusion;
    const float3 c_spec = lerp(kSpecularCoefficient, albedo, metallic)  * ambientOcclusion;

    // Output color
    float3 acc_color = 0;                         

    // Accumulate light values
    for (int i = 0; i < numLights; i++)
    {
        // light vector (to light)
        const float3 L = normalize(-lightDirection[i]);

        // Half vector
        const float3 H = normalize(L + V);

        // products
        const float NdotL = saturate(dot(N, L));
        const float LdotH = saturate(dot(L, H));
        const float NdotH = saturate(dot(N, H));

        // Diffuse & specular factors
        float diffuse_factor = Diffuse_Burley(NdotL, NdotV, LdotH, roughness);
        float3 specular      = Specular_BRDF(alpha, c_spec, NdotV, NdotL, LdotH, NdotH);

        // Directional light
        acc_color += NdotL * lightColor[i] * (((c_diff * diffuse_factor) + specular));
    }

    // Add diffuse irradiance
    float3 diffuse_env = Diffuse_IBL(N);
    acc_color += c_diff * diffuse_env;

    // Add specular radiance 
    float3 specular_env = Specular_IBL(N, V, roughness);
    acc_color += c_spec * specular_env;

    return acc_color;
}
