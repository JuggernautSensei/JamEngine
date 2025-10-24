/////////////////////////////
///          FWD          ///
/////////////////////////////

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
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
};

///////////////////////////////////////
///          VERTEX SHADER          ///
///////////////////////////////////////

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    float4 posW = mul(cb_world, float4(input.posL, 1.0f));
    output.posH = mul(cb_projView, posW);
    return output;
}

//////////////////////////////////////
///          PIXEL SHADER          ///
//////////////////////////////////////

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return cb_color;
}
