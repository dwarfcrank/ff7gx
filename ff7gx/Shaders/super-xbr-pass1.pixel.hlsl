#include "super-xbr-params.inc"

/* COMPATIBILITY
   - HLSL compilers
   - Cg   compilers
*/

/*
   
  *******  Super XBR Shader - pass1  *******
   
  Copyright (c) 2015 Hyllian - sergiogdb@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  Slightly modified to compile with the VS2017 HLSL compiler.
*/

#define wp1  8.0
#define wp2  0.0
#define wp3  0.0
#define wp4  0.0
#define wp5  0.0
#define wp6  0.0

#define weight1 (XBR_WEIGHT*1.75068/10.0)
#define weight2 (XBR_WEIGHT*1.29633/10.0/2.0)

const static float3 Y = float3(.2126, .7152, .0722);

float RGBtoYUV(float3 color)
{
    return dot(color, Y);
}

float df(float A, float B)
{
    return abs(A - B);
}


/*
                              P1
     |P0|B |C |P1|         C     F4          |a0|b1|c2|d3|
     |D |E |F |F4|      B     F     I4       |b0|c1|d2|e3|   |e1|i1|i2|e2|
     |G |H |I |I4|   P0    E  A  I     P3    |c0|d1|e2|f3|   |e3|i3|i4|e4|
     |P2|H5|I5|P3|      D     H     I5       |d0|e1|f2|g3|
                           G     H5
                              P2
*/


float d_wd(float b0, float b1, float c0, float c1, float c2, float d0, float d1, float d2, float d3, float e1, float e2, float e3, float f2, float f3)
{
    return (wp1 * (df(c1, c2) + df(c1, c0) + df(e2, e1) + df(e2, e3)) + wp2 * (df(d2, d3) + df(d0, d1)) + wp3 * (df(d1, d3) + df(d0, d2)) + wp4 * df(d1, d2) + wp5 * (df(c0, c2) + df(e1, e3)) + wp6 * (df(b0, b1) + df(f2, f3)));
}

float hv_wd(float i1, float i2, float i3, float i4, float e1, float e2, float e3, float e4)
{
    return (wp4 * (df(i1, i2) + df(i3, i4)) + wp1 * (df(i1, e1) + df(i2, e2) + df(i3, e3) + df(i4, e4)));
}

float3 min4(float3 a, float3 b, float3 c, float3 d)
{
    return min(a, min(b, min(c, d)));
}
float3 max4(float3 a, float3 b, float3 c, float3 d)
{
    return max(a, max(b, max(c, d)));
}

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
 
float4 main_fragment(in out_vertex VAR, uniform sampler2D s0 : TEXUNIT0, uniform sampler2D prevTexture : TEXUNIT1, uniform prev PASSPREV2, uniform input IN) : COLOR
{
    //Skip pixels on wrong grid
    float2 fp = frac(VAR.texCoord * IN.texture_size);
    float2 dir = fp - float2(0.5, 0.5);
    if ((dir.x * dir.y) > 0.0)
        return (fp.x > 0.5) ? tex2D(s0, VAR.texCoord) : tex2D(prevTexture, VAR.texCoord);

    float2 g1 = (fp.x > 0.5) ? float2(0.5 / IN.texture_size.x, 0.0) : float2(0.0, 0.5 / IN.texture_size.y);
    float2 g2 = (fp.x > 0.5) ? float2(0.0, 0.5 / IN.texture_size.y) : float2(0.5 / IN.texture_size.x, 0.0);

    float3 P0 = tex2D(prevTexture, VAR.texCoord - 3.0 * g1).xyz;
    float3 P1 = tex2D(s0, VAR.texCoord - 3.0 * g2).xyz;
    float3 P2 = tex2D(s0, VAR.texCoord + 3.0 * g2).xyz;
    float3 P3 = tex2D(prevTexture, VAR.texCoord + 3.0 * g1).xyz;

    float3 B = tex2D(s0, VAR.texCoord - 2.0 * g1 - g2).xyz;
    float3 C = tex2D(prevTexture, VAR.texCoord - g1 - 2.0 * g2).xyz;
    float3 D = tex2D(s0, VAR.texCoord - 2.0 * g1 + g2).xyz;
    float3 E = tex2D(prevTexture, VAR.texCoord - g1).xyz;
    float3 F = tex2D(s0, VAR.texCoord - g2).xyz;
    float3 G = tex2D(prevTexture, VAR.texCoord - g1 + 2.0 * g2).xyz;
    float3 H = tex2D(s0, VAR.texCoord + g2).xyz;
    float3 I = tex2D(prevTexture, VAR.texCoord + g1).xyz;

    float3 F4 = tex2D(prevTexture, VAR.texCoord + g1 - 2.0 * g2).xyz;
    float3 I4 = tex2D(s0, VAR.texCoord + 2.0 * g1 - g2).xyz;
    float3 H5 = tex2D(prevTexture, VAR.texCoord + g1 + 2.0 * g2).xyz;
    float3 I5 = tex2D(s0, VAR.texCoord + 2.0 * g1 + g2).xyz;

    float b = RGBtoYUV(B);
    float c = RGBtoYUV(C);
    float d = RGBtoYUV(D);
    float e = RGBtoYUV(E);
    float f = RGBtoYUV(F);
    float g = RGBtoYUV(G);
    float h = RGBtoYUV(H);
    float i = RGBtoYUV(I);

    float i4 = RGBtoYUV(I4);
    float p0 = RGBtoYUV(P0);
    float i5 = RGBtoYUV(I5);
    float p1 = RGBtoYUV(P1);
    float h5 = RGBtoYUV(H5);
    float p2 = RGBtoYUV(P2);
    float f4 = RGBtoYUV(F4);
    float p3 = RGBtoYUV(P3);

    /* Calc edgeness in diagonal directions. */
    float d_edge = (d_wd(d, b, g, e, c, p2, h, f, p1, h5, i, f4, i5, i4) - d_wd(c, f4, b, f, i4, p0, e, i, p3, d, h, i5, g, h5));

    /* Calc edgeness in horizontal/vertical directions. */
    float hv_edge = (hv_wd(f, i, e, h, c, i5, b, h5) - hv_wd(e, f, h, i, d, f4, g, i4));

    float limits = XBR_EDGE_STR + 0.000001;
    float edge_strength = smoothstep(0.0, limits, abs(d_edge));

    /* Filter weights. Two taps only. */
    float4 w1 = float4(-weight1, weight1 + 0.5, weight1 + 0.5, -weight1);
    float4 w2 = float4(-weight2, weight2 + 0.25, weight2 + 0.25, -weight2);

    /* Filtering and normalization in four direction generating four colors. */
    float3 c1 = mul(w1, float4x3(P2, H, F, P1));
    float3 c2 = mul(w1, float4x3(P0, E, I, P3));
    float3 c3 = mul(w2, float4x3(D + G, E + H, F + I, F4 + I4));
    float3 c4 = mul(w2, float4x3(C + B, F + E, I + H, I5 + H5));

    /* Smoothly blends the two strongest directions (one in diagonal and the other in vert/horiz direction). */
    float3 color = lerp(lerp(c1, c2, step(0.0, d_edge)), lerp(c3, c4, step(0.0, hv_edge)), 1 - edge_strength);

    /* Anti-ringing code. */
    float3 min_sample = min4(E, F, H, I) + (1 - XBR_ANTI_RINGING) * lerp((P2 - H) * (F - P1), (P0 - E) * (I - P3), step(0.0, d_edge));
    float3 max_sample = max4(E, F, H, I) - (1 - XBR_ANTI_RINGING) * lerp((P2 - H) * (F - P1), (P0 - E) * (I - P3), step(0.0, d_edge));
    color = clamp(color, min_sample, max_sample);

    return float4(color, 1.0);
}