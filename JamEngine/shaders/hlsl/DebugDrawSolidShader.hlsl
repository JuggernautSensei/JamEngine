/////////////////////////////
///          FWD          ///
/////////////////////////////

static const float3 k_diffuseLight = float3(1.0f, 1.0f, 1.0f);
static const float3 k_ambientLight = float3(77.f / 255.f, 67.f / 255.f, 82.f / 255.f) * 0.1f;
static const float3 k_specularLight = float3(1.0f, 1.0f, 1.0f);
static const float k_specularPower = 32.0f;
static const float3 k_lightDirection = float3(1.f, -2.f, 1.f);

cbuffer CB_DEBUGDRAW
{
    float4x4 cb_projView;
    float4x4 cb_world;
    float4x4 cb_worldInvTrans;
    float4 cb_color;
    float3 cb_eyePos;
    float pad1;
};

struct VS_INPUT
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : TEXCOORD0;
    float3 normalW : TEXCOORD1;
};

///////////////////////////////////////
///          VERTEX SHADER          ///
///////////////////////////////////////

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posW = mul(cb_world, float4(input.posL, 1.0f)).xyz;
    output.posH = mul(cb_projView, float4(output.posW, 1.f));
    output.normalW = mul(cb_worldInvTrans, float4(input.normalL, 0.0f)).xyz;
    output.normalW = normalize(output.normalW);
    return output;
}

//////////////////////////////////////
///          PIXEL SHADER          ///
//////////////////////////////////////

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float3 E = normalize(cb_eyePos - input.posW);
    float3 L = normalize(-k_lightDirection);
    float3 N = normalize(input.normalW);
    float3 H = normalize(E + L);

    float nDotL = max(dot(N, L), 0.0f);
    float nDotH = max(dot(N, H), 0.0f);
    float4 factors = lit(nDotL, nDotH, k_specularPower);

    float3 ambient = k_ambientLight * cb_color.rgb;
    float3 diffuse = k_diffuseLight * cb_color.rgb * factors.y;
    float3 specular = k_specularLight * factors.z;

    return float4(ambient + diffuse + specular, 1.0f);
}
