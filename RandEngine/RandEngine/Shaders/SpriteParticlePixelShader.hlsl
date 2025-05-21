Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
    float4 Color : COLOR;
    bool hasTexture : TEXCOORD1;
};

float4 mainPS(PS_Input Input) : SV_TARGET
{
    if (!Input.hasTexture)
        return Input.Color;
    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);
    float4 TextureColor = Texture.Sample(Sampler, Input.UV);
    float threshold = 0.01f;

    if (max(max(TextureColor.r, TextureColor.g), TextureColor.b) < threshold || TextureColor.a < 0.1f)
    {
        discard;
    }
    FinalColor = TextureColor * Input.Color;
    return FinalColor;
}
