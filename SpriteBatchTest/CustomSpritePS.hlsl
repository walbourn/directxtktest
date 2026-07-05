// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929

Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

float4 main(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    return Texture.Sample(TextureSampler, texCoord) * color;
}
