struct prev
{
    float2 video_size;
    float2 texture_size;
};
 
struct input
{
    float2 video_size;
    float2 texture_size;
    float2 output_size;
    float frame_count;
    float frame_direction;
    float frame_rotation;
};
 
 
struct out_vertex
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};
 
/*    VERTEX_SHADER    */
out_vertex main_vertex
(
    float4 position : POSITION,
    float4 color : COLOR,
    float2 texCoord : TEXCOORD0,
 
    uniform float4x4 modelViewProj,
    uniform prev PASSPREV2,
    uniform input IN
)
{
    out_vertex OUT =
    {
        mul(modelViewProj, position),
        color,
        texCoord,
    };
 
    return OUT;
}