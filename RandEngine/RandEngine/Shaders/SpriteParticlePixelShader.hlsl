Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
    float4 Color : COLOR;
};

float4 mainPS(PS_Input Input) : SV_TARGET
{
    float4 TextureColor = Texture.Sample(Sampler, Input.UV);
	return TextureColor * Input.Color;
}
