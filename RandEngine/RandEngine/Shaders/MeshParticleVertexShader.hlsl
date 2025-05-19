#include "ShaderRegisters.hlsl"

struct VS_InstanceData   
{
    float4 Color : COLOR;
    float4 Transform0 : TEXCOORD0;
    float4 Transform1 : TEXCOORD1;
    float4 Transform2 : TEXCOORD2;
    float4 Velocity : TEXCOORD3; 
    int4 SubUVParams : TEXCOORD4;
    float SubUVLerp : TEXCOORD5;
    float RelativeTime : TEXCOORD6;
};

float4 mainVS(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}
