
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

cbuffer FlagConstants : register(b2)
{
    bool IsLit;
    float3 flagPad0;
}

cbuffer SubMeshConstants : register(b3)
{
    bool IsSelectedSubMesh;
    float3 SubMeshPad0;
}

cbuffer TextureConstants : register(b4)
{
    float2 UVOffset;
    float2 TexturePad0;
}

cbuffer DiffuseMultiplierConstants : register(b6)
{
    float DiffuseMultiplier;
    float3 DiffuseOverrideColor;
}

#ifdef LIGHTING_MODEL_PBR
#include "LightPBR.hlsl"
#else
#include "Light.hlsl"
#endif

float4 mainPS(PS_INPUT_StaticMesh Input) : SV_Target
{
    // Diffuse
    float3 DiffuseColor = Material.DiffuseColor;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        float4 DiffuseColor4 = MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(MaterialSamplers[TEXTURE_SLOT_DIFFUSE], Input.UV);
        if (DiffuseColor4.a < 0.1f)
        {
            discard;
        }
        DiffuseColor = DiffuseColor4.rgb;
        // W08
        if (DiffuseMultiplier > 0.1)
        {
            float3 OtherColor = MaterialTextures[TEXTURE_SLOT_AMBIENT].Sample(MaterialSamplers[TEXTURE_SLOT_AMBIENT], Input.UV).rgb;
            DiffuseColor = lerp(DiffuseColor, OtherColor, DiffuseMultiplier * DiffuseMultiplier);
        }
        //
    }
    
    // Normal
    float3 WorldNormal = normalize(Input.WorldNormal);
    if (Material.TextureFlag & TEXTURE_FLAG_NORMAL)
    {
        float3 Tangent = normalize(Input.WorldTangent.xyz);
        float Sign = Input.WorldTangent.w;
        float3 BiTangent = cross(WorldNormal, Tangent) * Sign;

        float3x3 TBN = float3x3(Tangent, BiTangent, WorldNormal);
        
        float3 Normal = MaterialTextures[TEXTURE_SLOT_NORMAL].Sample(MaterialSamplers[TEXTURE_SLOT_NORMAL], Input.UV).rgb;
        Normal = normalize(2.f * Normal - 1.f);
        WorldNormal = normalize(mul(Normal, TBN));
    }

#ifndef LIGHTING_MODEL_PBR
    // Specular Color
    float3 SpecularColor = Material.SpecularColor;
    if (Material.TextureFlag & TEXTURE_FLAG_SPECULAR)
    {
        SpecularColor = MaterialTextures[TEXTURE_SLOT_SPECULAR].Sample(MaterialSamplers[TEXTURE_SLOT_SPECULAR], Input.UV).rgb;
    }

    // Specular Exponent or Glossiness
    float Shininess = Material.Shininess;
    if (Material.TextureFlag & TEXTURE_FLAG_SHININESS)
    {
        Shininess = MaterialTextures[TEXTURE_SLOT_SHININESS].Sample(MaterialSamplers[TEXTURE_SLOT_SHININESS], Input.UV).r;
        Shininess = 1000 * Shininess * Shininess; // y = 1000 * x ^ 2
    }
#endif

    // Emissive Color
    float3 EmissiveColor = Material.EmissiveColor;
    if (Material.TextureFlag & TEXTURE_FLAG_EMISSIVE)
    {
        EmissiveColor = MaterialTextures[TEXTURE_SLOT_EMISSIVE].Sample(MaterialSamplers[TEXTURE_SLOT_EMISSIVE], Input.UV).rgb;
    }

#ifdef LIGHTING_MODEL_PBR
    // Metallic
    float Metallic = Material.Metallic;
    if (Material.TextureFlag & TEXTURE_FLAG_METALLIC)
    {
        Metallic = MaterialTextures[TEXTURE_SLOT_METALLIC].Sample(MaterialSamplers[TEXTURE_SLOT_METALLIC], Input.UV).b;
    }

    // Roughness
    float Roughness = Material.Roughness;
    if (Material.TextureFlag & TEXTURE_FLAG_ROUGHNESS)
    {
        Roughness = MaterialTextures[TEXTURE_SLOT_ROUGHNESS].Sample(MaterialSamplers[TEXTURE_SLOT_ROUGHNESS], Input.UV).g;
    }
#endif

    // Begin for Tile based light culled result
    // 현재 픽셀이 속한 타일 계산 (input.position = 화면 픽셀좌표계)
    uint2 PixelCoord = uint2(Input.Position.xy);
    uint2 TileCoord = PixelCoord / TileSize; // 각 성분별 나눔
    uint TilesX = ScreenSize.x / TileSize.x; // 한 행에 존재하는 타일 수
    uint FlatTileIndex = TileCoord.x + TileCoord.y * TilesX;
    // End for Tile based light culled result
    
    // Lighting
    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);
    if (IsLit)
    {
        float3 LitColor = float3(0, 0, 0);
#ifdef LIGHTING_MODEL_GOURAUD
        LitColor = Input.Color.rgb;
#elif defined(LIGHTING_MODEL_PBR)
        LitColor = Lighting(Input.WorldPosition, WorldNormal, ViewWorldLocation, DiffuseColor, Metallic, Roughness);
#else
        LitColor = Lighting(Input.WorldPosition, WorldNormal, ViewWorldLocation, DiffuseColor, SpecularColor, Shininess, FlatTileIndex);
#endif
        LitColor += EmissiveColor * 5.f; // 5는 임의의 값
        FinalColor = float4(LitColor, 1);
    }
    else
    {
        FinalColor = float4(DiffuseColor, 1);
    }
    
    if (bIsSelected)
    {
        FinalColor += float4(0.01, 0.01, 0.0, 1);
    }

    return FinalColor;
}
