// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929

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

VS_Output main(float4 position : SV_Position,
    float4 color : COLOR0,
    float2 texCoord : TEXCOORD0)
{
    VS_Output output;
    output.position = mul(position, MatrixTransform);

    // Pass through to do lighting in pixel shader.
    output.color = color;
    output.texCoord = texCoord;
    return output;
}
