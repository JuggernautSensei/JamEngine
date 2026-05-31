#include "ImageFilterCommon.hlsl"

#define TONE_MAPPING_NONE        0
#define TONE_MAPPING_LINEAR      1
#define TONE_MAPPING_EXPONENTIAL 2
#define TONE_MAPPING_REINHARD    3
#define TONE_MAPPING_UNCHARTED2  4
#define TONE_MAPPING_MGSV        5
#define TONE_MAPPING_UCHIMURA    6
#define TONE_MAPPING_FILMIC      7
#define TONE_MAPPING_ACES        8
#define TONE_MAPPING_PBRNEUTRAL  9
#define TONE_MAPPING_FLIM        10
#define TONE_MAPPING_AGX         11

// cb_fparams.x : exposure
// cb_fparams.y : gamma
// cb_iparams.x : tone mapping type

float3 Linear(float3 x)
{
    return x;
}

float3 Exponential(float3 x)
{
    return sqrt(1.f - exp(-Square(x)));
}

float3 Reinhard(float3 x)
{
    return 1.f - 1.f / (1.f + x);
}

float3 Uncharted2(float3 x)
{
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    const float W = 11.2f;
    const float exposureBias = 2.f;
    x *= exposureBias;
    x = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    x /= white;
    return x;
}

float3 MGSV(float3 x)
{
    const float A = 0.6f;
    const float B = 0.45f;
    float3 t = step(A, x);
    float3 y = min(1.f, A + B - Square(B) / (x - A + B));
    return lerp(x, y, t);
}

float3 Uchimura(float3 x)
{
    const float P = 1.0f;
    const float a = 1.0f;
    const float m = 0.22f;
    const float l = 0.4f;
    const float c = 1.33f;
    const float b = 0.0f;

    float l0 = ((P - m) * l) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;
    float3 w0 = 1.0f - smoothstep(0.0f, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0f - w0 - w2;
    float3 T = m * pow(x / m, c) + b;
    float3 S = P - (P - S1) * exp(CP * (x - S0));
    float3 L = m + a * (x - m);
    return T * w0 + L * w1 + S * w2;
}

float3 Filmic(float3 x)
{
    x = max(0.f, x - 0.004f);
    x = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
    return x; // gamma 내장
}

float3 ACES(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 PBRNeutral(float3 x)
{
    const float startCompression = 0.8f - 0.04f;
    const float desaturation = 0.15f;

    float minC = min(x.r, min(x.g, x.b));
    float offset = minC < 0.08f ? minC - 6.25f * Square(minC) : 0.04f;
    x -= offset;

    float peak = max(x.r, max(x.g, x.b));
    if (peak >= startCompression)
    {
        float d = 1.f - startCompression;
        float newPeak = 1.f - Square(d) / (peak + d - startCompression);
        x *= newPeak / peak;
        float g = 1.f - 1.f / (desaturation * (peak - newPeak) + 1.f);
        x = lerp(x, newPeak, g);
    }
    return x;
}

float3 AGXDefaultContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return 15.5f * x4 * x2
          - 40.14f * x4 * x
          + 31.96f * x4
          - 6.868f * x2 * x
          + 0.4298f * x2
          + 0.1191f * x
          - 0.00232f;
}

float3 AGXTonemap(float3 val)
{
    const float3x3 agxMat = float3x3(
        0.842479062253094f, 0.0423282422610123f, 0.0423756549057051f,
        0.0784335999999992f, 0.878468636469772f, 0.0784336f,
        0.0792237451477643f, 0.0791661274605434f, 0.879142973793104f
    );
    const float minEv = -12.47393f;
    const float maxEv = 4.026069f;

    val = mul(agxMat, val);
    val = clamp(log2(val), minEv, maxEv);
    val = (val - minEv) / (maxEv - minEv);
    return AGXDefaultContrastApprox(val);
}

float3 AGXEotf(float3 val)
{
    const float3x3 agxMatInv = float3x3(
         1.19687900512017f, -0.0528968517574562f, -0.0529716355144438f,
        -0.0980208811401368f, 1.15190312990417f, -0.0980434501171241f,
        -0.0990297440797205f, -0.0989611768448433f, 1.15107367264116f
    );
    return mul(agxMatInv, val);
}

float3 AGXLook(float3 val)
{
    // Punchy look
    const float3 power = float3(1.35f, 1.35f, 1.35f);
    const float sat = 1.4f;

    val = pow(val, power);

    const float3 lw = float3(0.2126f, 0.7152f, 0.0722f);
    float luma = dot(val, lw);
    return luma + sat * (val - luma);
}

float3 AGX(float3 x)
{
    return AGXEotf(AGXLook(AGXTonemap(x))); // gamma 내장
}

float3 FlimRh(float3 r)
{
    float a = max(r.x, max(r.y, r.z));
    float i = min(r.x, min(r.y, r.z));
    float d = a - i;
    float v = a;
    float s, h;
    if (a != 0.f)
    {
        s = d / a;
    }
    else
    {
        s = 0.f;
    }
    if (s == 0.f)
    {
        h = 0.f;
    }
    else
    {
        float3 c = (a - r) / d;
        if (r.x == a)
        {
            h = c.z - c.y;
        }
        else if (r.y == a)
        {
            h = 2.f + c.x - c.z;
        }
        else
        {
            h = 4.f + c.y - c.x;
        }
        h /= 6.f;
        if (h < 0.f)
        {
            h += 1.f;
        }
    }
    return float3(h, s, v);
}

float3 FlimHr(float3 w)
{
    float h = w.x, s = w.y, v = w.z;
    float3 outColor = float3(v, v, v);
    if (s != 0.f)
    {
        if (h == 1.f)
        {
            h = 0.f;
        }
        h *= 6.f;
        int idx = int(floor(h));
        float f = h - float(idx);
        float p = v * (1.f - s);
        float q = v * (1.f - s * f);
        float t = v * (1.f - s * (1.f - f));
        if (idx == 0)
        {
            outColor = float3(v, t, p);
        }
        else if (idx == 1)
        {
            outColor = float3(q, v, p);
        }
        else if (idx == 2)
        {
            outColor = float3(p, v, t);
        }
        else if (idx == 3)
        {
            outColor = float3(p, q, v);
        }
        else if (idx == 4)
        {
            outColor = float3(t, p, v);
        }
        else
        {
            outColor = float3(v, p, q);
        }
    }
    return outColor;
}

float FlimF0(float v, float s, float e)
{
    return saturate((v - s) / (e - s));
}
float FlimFw(float v, float s, float e)
{
    return s + fmod(v - s, e - s);
}
float FlimFa(float3 c)
{
    return (c.x + c.y + c.z) / 3.f;
}
float FlimFs(float3 c)
{
    return c.x + c.y + c.z;
}
float FlimFr(float3 c)
{
    return max(c.x, max(c.y, c.z));
}

float3 FlimGe(float p, float s, float r, float m)
{
    float3 o = FlimHr(float3(FlimFw(p + r / 360.f, 0.f, 1.f), 1.f / s, 1.f));
    o /= FlimFs(o);
    o *= m;
    return o;
}

float FlimSs(float v, float x, float y, float sx, float sy)
{
    v = saturate(v);
    x = saturate(x);
    y = saturate(y);
    sx = saturate(sx);
    sy = saturate(sy);
    float slope = (sy - y) / (sx - x);
    float result = 0.f;
    if (v < x)
    {
        float tt = slope * x / y;
        result = y * pow(v / x, tt);
    }
    else if (v < sx)
    {
        result = slope * v + (y - slope * x);
    }
    else
    {
        float sp = -slope / (((sx - 1.f) / Square(1.f - sx)) * (1.f - sy));
        result = (1.f - pow(1.f - (v - sx) / (1.f - sx), sp)) * (1.f - sy) + sy;
    }
    return result;
}

float FlimDm(float m, float d)
{
    const float lm = -10.f, lx = 22.f;
    const float tx = 0.44f, ty = 0.28f, sx = 0.591f, sy = 0.779f;
    float o = pow(2.f, lm);
    float f = FlimF0(log2(m + o), lm, lx);
    f = FlimSs(f, tx, ty, sx, sy);
    f *= d;
    f = pow(2.f, -f);
    return saturate(f);
}

float3 FlimRl(float3 c, float3 st, float3 dt, float d)
{
    float3 sn = st / FlimFs(st);
    float3 dn = dt / FlimFr(dt);
    float m = dot(c, sn);
    float f = FlimDm(m, d);
    return lerp(dn, 1.f, f);
}

float3 FlimRd(float3 c, float e, float d)
{
    c *= pow(2.f, e);
    float3 r = FlimRl(c, float3(0, 0, 1), float3(1, 1, 0), d);
    r *= FlimRl(c, float3(0, 1, 0), float3(1, 0, 1), d);
    r *= FlimRl(c, float3(1, 0, 0), float3(0, 1, 1), d);
    return r;
}

float3 FlimNp(float3 c, float3 b)
{
    const float fe = 6.f, fd = 5.f, pfe = 6.f, pfd = 27.5f;
    c = FlimRd(c, fe, fd);
    c *= b;
    c = FlimRd(c, pfe, pfd);
    return c;
}

float3 FlimBs(float3 c, float h, float s, float v)
{
    float3 r = FlimRh(c);
    r.x = frac(r.x + h + 0.5f);
    r.y = saturate(r.y * s);
    r.z = r.z * v;
    return FlimHr(r);
}

float3 Flim(float3 c)
{
    const float pe = 4.3f;
    const float gr = 1.05f, gg = 1.12f, gb = 1.045f;
    const float rr = 0.5f, rg = 2.f, br = 0.1f;
    const float rm = 1.f, gm = 1.f, bm = 1.f;
    const float ms = 1.02f;
    const float3 pb = 1.f, pf = 1.f, pff = 1.f;

    c = max(c, 0.f);
    c *= pow(2.f, pe);
    c = min(c, 5000.f);

    float3x3 x = float3x3(
        FlimGe(0.f, gr, rr, rm),
        FlimGe(1.f / 3.f, gg, rg, gm),
        FlimGe(2.f / 3.f, gb, br, bm)
    );
    float3x3 inv = transpose(float3x3( // HLSL inverse 없으므로 3x3 수동
        x[1].y * x[2].z - x[1].z * x[2].y, -(x[0].y * x[2].z - x[0].z * x[2].y), x[0].y * x[1].z - x[0].z * x[1].y,
        -(x[1].x * x[2].z - x[1].z * x[2].x), x[0].x * x[2].z - x[0].z * x[2].x, -(x[0].x * x[1].z - x[0].z * x[1].x),
        x[1].x * x[2].y - x[1].y * x[2].x, -(x[0].x * x[2].y - x[0].y * x[2].x), x[0].x * x[1].y - x[0].y * x[1].x
    ));
    float det = x[0].x * (x[1].y * x[2].z - x[1].z * x[2].y)
              - x[0].y * (x[1].x * x[2].z - x[1].z * x[2].x)
              + x[0].z * (x[1].x * x[2].y - x[1].y * x[2].x);
    inv /= det;

    float3 b = mul(pb, x);
    const float g = 1e7f;
    float3 w = FlimNp(g, b);

    c = mul(c, x);
    c = FlimNp(c, b);
    c = mul(c, inv);
    c = max(c, 0.f);
    c /= w;

    float3 f = FlimNp(0.f, b);
    f /= w;
    float avg = FlimFa(f) * 1000.f;
    float m2 = FlimFa(c);
    c = c * (saturate((m2 - avg / 1000.f) / (1.f - 0.f)) / m2);

    c = saturate(c);
    float mv = FlimFa(c);
    float mixFac = (mv < 0.5f) ? FlimF0(mv, 0.05f, 0.5f) : FlimF0(mv, 0.95f, 0.5f);
    c = lerp(c, FlimBs(c, 0.5f, ms, 1.f), mixFac);
    return saturate(c);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = SAMPLE_TEXTURE2D_INPUT0(s_linearClamp, input.texcoord).rgb;
    float exposure = cb_fparams.x;
    float gamma = cb_fparams.y;

    color *= pow(2.f, exposure);

   // tonemapping
    bool bApplyGamma = true;
    int tmType = cb_iparams.z;
    if (tmType == TONE_MAPPING_NONE)
    {
        return float4(color, 1.f);
    }
    else if (tmType == TONE_MAPPING_LINEAR)
    {
        color = Linear(color);
    }
    else if (tmType == TONE_MAPPING_EXPONENTIAL)
    {
        color = Exponential(color);
    }
    else if (tmType == TONE_MAPPING_REINHARD)
    {
        color = Reinhard(color);
    }
    else if (tmType == TONE_MAPPING_UNCHARTED2)
    {
        color = Uncharted2(color);
    }
    else if (tmType == TONE_MAPPING_MGSV)
    {
        color = MGSV(color);
    }
    else if (tmType == TONE_MAPPING_UCHIMURA)
    {
        color = Uchimura(color);
    }
    else if (tmType == TONE_MAPPING_FILMIC)
    {
        color = Filmic(color);
        bApplyGamma = false;
    }
    else if (tmType == TONE_MAPPING_ACES)
    {
        color = ACES(color);
    }
    else if (tmType == TONE_MAPPING_PBRNEUTRAL)
    {
        color = PBRNeutral(color);
    }
    else if (tmType == TONE_MAPPING_FLIM)
    {
        color = Flim(color);
    }
    else if (tmType == TONE_MAPPING_AGX)
    {
        color = AGX(color);
        bApplyGamma = false;
    }

   // gamma
    if (bApplyGamma)
    {
        color = pow(color, 1.f / gamma);
    }

    return float4(color, 1.f);
}