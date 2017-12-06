#include "background.hlsli"

sampler2D background;

float4 main(in PsInput vertex): COLOR0
{
    float4 texColor = tex2D(background, vertex.texcoord.xy);

    if (texColor.a == 0.0f)
    {
        discard;
    }

    float z = (vertex.position.z - 0.9f) * 10.0f;

    return float4(texColor.rgb, z);
}