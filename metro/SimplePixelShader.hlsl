Texture2D SimpleTexture : register(t0);
SamplerState SimpleSampler : register(s0);

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXCOORD0;
};

float4 SimplePixelShader(PixelShaderInput input) : SV_TARGET
{
    float3 lightDirection = normalize(float3(1, -1, 0));
    return SimpleTexture.Sample(SimpleSampler, input.tex) * (0.1f * saturate(dot(input.norm, -lightDirection)) + 0.9f);
}
