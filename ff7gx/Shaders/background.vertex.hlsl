#include "background.hlsli"

float4x4 projection;

VsOutput main(VsInput input)
{
    VsOutput output;

    output.color = input.color;
    output.texcoord = input.texcoord;

    float w = 1.0f / input.position.w;
    float4 position = float4(input.position.xyz * w, w);

    position = mul(position, projection);

    output.position = position;
    output.newPosition = position;

    return output;
}