#include "ShaderRegisters.hlsl"

#ifdef LIGHTING_MODEL_GOURAUD
SamplerState DiffuseSampler : register(s0);

Texture2D DiffuseTexture : register(t0);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

#include "Light.hlsl"
#endif

struct VS_Input  
{
    float3 Position : POSITION;
    float4 MeshColor : COLOR;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MaterialIndex : MATERIAL_INDEX;
    
    float4 Color : COLOR1;
    float4 Transform0 : TEXCOORD1;
    float4 Transform1 : TEXCOORD2;
    float4 Transform2 : TEXCOORD3;
    float4 Velocity : TEXCOORD4; 
    int4 SubUVParams : TEXCOORD5;
    float SubUVLerp : TEXCOORD6;
    float RelativeTime : TEXCOORD7;
};

PS_INPUT_StaticMesh mainVS(VS_Input Input)
{
    PS_INPUT_StaticMesh Output;

    Output.Position = float4(Input.Position, 1.0);
    float4x4 Transform = float4x4(Input.Transform0, Input.Transform1, Input.Transform2, float4(0, 0, 0, 1));
    float3x3 Rotation = (float3x3) Transform;
    float3x3 InvTransform = transpose(Rotation);
    Output.Position = mul(Transform, Output.Position);
    Output.WorldPosition = Output.Position.xyz;
    
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.WorldNormal = mul(Input.Normal, InvTransform);

    // Begin Tangent
    float3 WorldTangent = mul(Input.Tangent.xyz, Rotation);
    WorldTangent = normalize(WorldTangent);
    WorldTangent = normalize(WorldTangent - Output.WorldNormal * dot(Output.WorldNormal, WorldTangent));

    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    // End Tangent
    
    Output.UV = Input.UV;
    Output.MaterialIndex = Input.MaterialIndex;

#ifdef LIGHTING_MODEL_GOURAUD
    float3 DiffuseColor = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        DiffuseColor = DiffuseTexture.SampleLevel(DiffuseSampler, Input.UV, 0).rgb;
    }
    float3 Diffuse = Lighting(Output.WorldPosition, Output.WorldNormal, ViewWorldLocation, DiffuseColor, Material.SpecularColor, Material.Shininess);
    Output.Color = float4(Diffuse.rgb, 1.0);
#else
    Output.Color = Input.Color;
#endif
    
    return Output;
}
