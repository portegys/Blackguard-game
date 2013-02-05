struct ColoredPixelShaderInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 ColoredPixelShader(ColoredPixelShaderInput input) : SV_TARGET
{
    return input.color;
}
