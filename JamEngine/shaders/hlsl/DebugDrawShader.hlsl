/////////////////////////////
///          FWD          ///
/////////////////////////////

// #define VARIANT_LINE

#define MATRIX row_major matrix

static const float3 k_diffuseLight = float3(1.0f, 1.0f, 1.0f);
static const float3 k_ambientLight = float3(77.f / 255.f, 67.f / 255.f, 82.f / 255.f) * 0.1f;
static const float3 k_specularLight = float3(1.0f, 1.0f, 1.0f);
static const float k_specularPower = 32.0f;
static const float3 k_lightDirection = float3(1.f, -2.f, 1.f);

Texture2D t_texture : register(t0);
SamplerState s_sampler : register(s0);

cbuffer CB_DEBUGDRAW
{
    MATRIX cb_viewProjMatrix;
    MATRIX cb_transformMatrix;
    MATRIX cb_transformInvTransposeMatrix;
    //
    float4 cb_color;
    //
    float3 cb_eyePosition;
    float pad1;
};

struct VS_INPUT
{
    float3 position : POSITION;

#ifndef VARIANT_LINE
    float3 normal : NORMAL;
#endif // VARIANT_LINE
};

struct PS_INPUT
{
    float4 position : SV_POSITION;

#ifndef VARIANT_LINE
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
#endif // VARIANT_LINE
};

///////////////////////////////////////
///          VERTEX SHADER          ///
///////////////////////////////////////

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

#ifdef VARIANT_LINE
    float4 worldPos = mul(float4(input.position, 1.0f), cb_transformMatrix);
    output.position = mul(worldPos, cb_viewProjMatrix);
#else
    output.worldPosition = mul(float4(input.position, 1.0f), cb_transformMatrix);
    output.position = mul(output.worldPosition, cb_viewProjMatrix);
    output.normal = normalize(mul(float4(input.normal, 0.0f), cb_transformInvTransposeMatrix).xyz);
#endif // VARIANT_LINE
    return output;
}

//////////////////////////////////////
///          PIXEL SHADER          ///
//////////////////////////////////////

float4 PSMain(PS_INPUT input) : SV_TARGET
{
#ifdef VARIANT_LINE
        return cb_color;
#else
    float3 E = normalize(cb_eyePosition - input.worldPosition);
    float3 L = normalize(-k_lightDirection);
    float N = normalize(input.normal);
    float H = normalize(E + L);
    float nDotL = max(dot(N, L), 0.0f);
    float nDotH = max(dot(N, H), 0.0f);
    float4 factors = lit(nDotL, nDotH, k_specularPower);

    float3 ambient = k_ambientLight * cb_color.rgb;
    float3 diffuse = k_diffuseLight * cb_color.rgb * factors.y;
    float3 specular = k_specularLight * factors.z;

    return float4(ambient + diffuse + specular, 1.f);
#endif // VARIANT_LINE
}