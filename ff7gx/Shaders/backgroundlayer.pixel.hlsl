#include "background.hlsli"

float layerDepth;
sampler2D background;
sampler2D backgroundPoint;

float4 main(in PsInput vertex): COLOR0
{
    float4 texColor = tex2D(background, vertex.texcoord.xy);
    float4 pointTexColor = tex2D(backgroundPoint, vertex.texcoord.xy);
    float pixelDepth = ceil(pointTexColor.a * 255.0f);

    if (pixelDepth > layerDepth)
    {
        discard;
    }

    return float4(texColor.rgb, 1.0f);
}