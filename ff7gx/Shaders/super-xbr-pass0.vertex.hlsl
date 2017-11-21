
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
    float4 t1 : TEXCOORD1;
    float4 t2 : TEXCOORD2;
    float4 t3 : TEXCOORD3;
    float4 t4 : TEXCOORD4;
};
 
/*    VERTEX_SHADER    */
out_vertex main_vertex
(
    float4 position : POSITION,
    float4 color : COLOR,
    float2 texCoord : TEXCOORD0,
 
    uniform float4x4 modelViewProj,
    uniform input IN
)
{
    float2 ps = float2(1.0 / IN.texture_size.x, 1.0 / IN.texture_size.y);
    float dx = ps.x;
    float dy = ps.y;

    out_vertex OUT =
    {
        mul(modelViewProj, position),
        color,
        texCoord,
        texCoord.xyxy + float4(-dx, -dy, 2.0 * dx, 2.0 * dy),
        texCoord.xyxy + float4(0, -dy, dx, 2.0 * dy),
        texCoord.xyxy + float4(-dx, 0, 2.0 * dx, dy),
        texCoord.xyxy + float4(0, 0, dx, dy)
    };
 
    return OUT;
}