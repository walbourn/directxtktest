// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929

Texture2D<float4> Texture : register(t0);
Texture2D<float4> NormalMap : register(t1);
sampler TextureSampler : register(s0);

cbuffer Parameters : register(b0)
{
    float4 LightDir;
    row_major float4x4 MatrixTransform;
};

struct VS_Output
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texCoord : TEXCOORD0;
};

float4 main(VS_Output input) : SV_Target0
{
    float4 base = Texture.Sample(TextureSampler, input.texCoord);
    float3 biasedNormal = NormalMap.Sample(TextureSampler, input.texCoord).rgb;

    // Assumes biased normal map using UNORM
    float3 normal = normalize(biasedNormal * 2.0f - 1.0f);

    float3 lightDir = normalize(LightDir.xyz);

    float diffuse = saturate(dot(normal, lightDir));

    float3 lighting = 0.3f + 0.7f * diffuse;

    return base * input.color * float4(lighting, 1.0f);
}
