//#include "Common.hlsl"

//struct VSInput
//{
//    float3 posL : POSITION;
//};

//struct PSInput
//{
//    float4 posH : SV_POSITION;
//    float3 posV : POSITION;
//};

//PSInput VSMain(VSInput input)
//{
//    PSInput output;
//    float4 posW = mul(float4(input.posL, 1.f), cb_world);

//}
//[maxvertexcount(3 * 6)]
//void GSMain(triangle GSInput input[3], inout TriangleStream<PSInput> triStream)
//{
//    [unroll]
//    for (int i = 0; i < 6; i++)
//    {
//        PSInput output;

//        [unroll]
//        for (int j = 0; j < 3; j++)
//        {
//            output.posV = mul(input[j].posW, cb_omniLightView[i]).xyz;
//            output.posH = mul(float4(output.posV, 1.f), cb_omniLightProj);
//            output.rtIndex = i;
//            triStream.Append(output);
//        }

//        triStream.RestartStrip();
//    }
//}
//#if defined(SHADOW_CAST_VSM)

//float2 PSMain(PSInput _input) : SV_TARGET
//{
//    float d2 = dot(_input.posV, _input.posV);
//    return float2(sqrt(d2), d2);
//}

//#elif defined(SHADOW_CAST_ESM)

//float PSMain(PSInput _input) : SV_TARGET
//{
//    float depth = _input.posV.z;
//    return exp(cb_esmExponent * (depth - 1.f));
//}

//#else

//float PSMain(PSInput _input) : SV_DEPTH
//{
//    float dist = length(_input.posV);
//    return dist / cb_lightFallOffEnd;
//}

//#endif