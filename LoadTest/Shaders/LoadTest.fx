//--------------------------------------------------------------------------------------
// File: d3d11.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer cbNeverChanges : register( b0 )
{
    matrix View;
};

cbuffer cbChangeOnResize : register( b1 )
{
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register( b2 )
{
    matrix World;
    float4 EyePosition;
    float4 vMeshColor;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : SV_Position;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_Position;
    float4 Tex : TEXCOORD0;
};


SamplerState samLinear : register( s0 );

//--------------------------------------------------------------------------------------
// 1D/2D Texture
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture1D txDiffuse1D : register( t0 );

PS_INPUT VS2D( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( float4(input.Pos, 1), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Tex = float4(input.Tex, 0, 0);
    
    return output;
}

float4 PS1D( PS_INPUT input) : SV_Target
{
    return txDiffuse1D.Sample( samLinear, input.Tex.x ) * vMeshColor;
}

float4 PS2D( PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample( samLinear, input.Tex.xy ) * vMeshColor;
}

//--------------------------------------------------------------------------------------
// Cubemap
//--------------------------------------------------------------------------------------
TextureCube cubeDiffuse : register( t0 );

PS_INPUT VSCube( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;

    float4 pos_ws = mul( float4(input.Pos, 1), World );
    
    float3 eyeVector = normalize( pos_ws.xyz - EyePosition.xyz );
    float3 worldNormal = normalize( mul( float4(input.Normal, 0), World ).xyz );
   
    output.Pos = mul( pos_ws, View );
    output.Pos = mul( output.Pos, Projection );
    output.Tex = float4( reflect( eyeVector, worldNormal ), 0 );
      
    return output;
}

float4 PSCube( PS_INPUT input) : SV_Target
{
    return cubeDiffuse.Sample( samLinear, input.Tex.xyz ) * vMeshColor;
}


//--------------------------------------------------------------------------------------
// Volume texture
//--------------------------------------------------------------------------------------
Texture3D txDiffuse3D : register ( t0 );

PS_INPUT VS3D( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;

    float4 pos_ws = mul( float4(input.Pos, 1), World );
    
    output.Pos = mul( pos_ws, View );
    output.Pos = mul( output.Pos, Projection );
  
    output.Tex = float4( pos_ws.xyz, 0 );
    
    return output;
}

float4 PS3D( PS_INPUT input) : SV_Target
{
    return txDiffuse3D.Sample( samLinear, input.Tex.xyz ) * vMeshColor;
}
