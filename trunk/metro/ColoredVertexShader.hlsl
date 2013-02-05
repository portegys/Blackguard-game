cbuffer SimpleConstantBuffer : register( b0 )
{
    matrix model;
    matrix view;
    matrix projection;
};

struct ColoredVertexShaderInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
};

struct ColoredPixelShaderInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

ColoredPixelShaderInput ColoredVertexShader(ColoredVertexShaderInput input)
{
    ColoredPixelShaderInput vertexShaderOutput;
    float4 pos = float4(input.pos, 1.0f);

    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);
    vertexShaderOutput.pos = pos;

    vertexShaderOutput.color = float4(input.color, 1.0f);

    return vertexShaderOutput;
}
