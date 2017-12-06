struct VsInput
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VsOutput
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD0;
    float4 newPosition : TEXCOORD1;
    float4 color : COLOR0;
};

struct PsInput
{
    float4 texcoord : TEXCOORD0;
    float4 position : TEXCOORD1;
    float4 color : COLOR0;
};
