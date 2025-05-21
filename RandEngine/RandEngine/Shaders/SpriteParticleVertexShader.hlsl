#include "ShaderRegisters.hlsl"

struct VS_Input
{
    float2 UV : TEXCOORD;
    float3 Position : POSITION;
    float RelativeTime : TEXCOORD1;
    float3 OldPosition : TEXCOORD2;
    float ParticleId : TEXCOORD3;
    float2 Size : TEXCOORD4;
    float Rotation : TEXCOORD5;
    float SubImageIndex : TEXCOORD6;
    float4 Color : COLOR;
};

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;   
    float4 Color : COLOR;
    bool hasTexture : TEXCOORD1;
};

cbuffer SpriteCameraConstant : register(b0)
{
    float3 CameraUp;
    bool hasTexture;
    float3 CameraRight;
    float Pad2;
}

static const float2 QuadOffsets[4] =
{
    float2(-1.0, 1.0),
    float2(1.0, 1.0),
    float2(1.0, -1.0),
    float2(-1.0, -1.0)
};


PS_Input mainVS(VS_Input Input, uint VertexID : SV_VertexID)
{
    PS_Input Output;
    
    float2 Offset = QuadOffsets[VertexID % 4];
    float2 ScaledOffset = Offset * Input.Size;
    
    float s = sin(Input.Rotation);
    float c = cos(Input.Rotation);
    
    float2 RotatedOffset = float2(
        ScaledOffset.x * c - ScaledOffset.y * s,
        ScaledOffset.x * s + ScaledOffset.y * c
    );
    
    float3 WorldPos = Input.Position + RotatedOffset.x * CameraRight + RotatedOffset.y * CameraUp;
    
    float4 ViewPos = mul(float4(WorldPos, 1.0), ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.UV = Input.UV;
    Output.Color = Input.Color;
    
    Output.hasTexture = hasTexture;
    
    return Output;
}
