#include "ShaderRegisters.hlsl"

struct VS_Input
{
    float3 Position : POSITION;
    float RelativeTime : TEXCOORD0;
    float3 OldPosition : TEXCOORD1;
    float ParticleId : TEXCOORD2;
    float2 Size : TEXCOORD3;
    float Rotation : TEXCOORD4;
    float SubImageIndex : TEXCOORD5;
    float4 Color : COLOR;
};

struct VS_Output
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};

cbuffer SpriteCameraConstant : register(b0)
{
    float3 CameraUp;
    float Pad1;
    float3 CameraRight;
    float Pad2;
}

static const float2 QuadUVs[4] =
{
    float2(0, 0), float2(1, 0), float2(1, 1), float2(0, 1)
};

static const float2 QuadOffsets[4] =
{
    float2(-1.0, -1.0),
    float2(1.0, -1.0),
    float2(1.0, 1.0),
    float2(-1.0, 1.0)
};


VS_Output mainVS(VS_Input Input, uint VertexID : SV_VertexID)
{
    VS_Output Output;
    
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
    Output.UV = QuadUVs[VertexID % 4];
    Output.Color = Input.Color;
    
    return Output;
}
