#include "ShaderBase.hlsl"

float PSmain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    // get world normal
    float4 normalRoughness = t_gBufferNormalRoughness.Sample(s_linearClamp, _input.texCoord0);
    float3 normal = normalize(normalRoughness.xyz);

    // get ndc pos
    float3 posInNdc;
    posInNdc.x = _input.texCoord0.x * 2.f - 1.f;
    posInNdc.y = -(_input.texCoord0.y * 2.f - 1.f);
    posInNdc.z = t_depth.Sample(s_pointClamp, _input.texCoord0).r;

    // world pos
    float4 posInC = mul(cb_viewProjInv, float4(posInNdc, 1.0f));
    float3 posInW = posInC.xyz / posInC.w;

    // screen dimensions
    uint screenWidth, screenHeight, dummy;
    t_gBufferNormalRoughness.GetDimensions(0, screenWidth, screenHeight, dummy);

    uint width, height, dummy2;
    t_ssaoNoise.GetDimensions(0, width, height, dummy2);

    // noise sampling 
    float2 noiseScale;
    noiseScale.x = screenWidth / (float) width;
    noiseScale.y = screenHeight / (float) height;

    // for kernel rotation
    float3 randomVec = t_ssaoNoise.Sample(s_pointWrap, _input.texCoord0 * noiseScale).xyz;

    // TBN
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = normalize(cross(normal, tangent));
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // sample kernel
    float occlusion = 0.0f;
    for (uint i = 0; i < JAM_SSAO_KERNEL_SIZE; i++)
    {
        // sample ssao kernel
        float3 kernelDir = cb_ssaoKernel[i].xyz;
        float3 sampleDirW = mul(TBN, kernelDir); // tangent -> world
        float3 samplePosInW = posInW + sampleDirW * cb_ssaoRadius; // offset position in world

        float4 samplePosInV = mul(cb_view, float4(samplePosInW, 1.0f));
        float4 samplePosInC = mul(cb_proj, samplePosInV);
        float2 samplePosInNdc = samplePosInC.xy / samplePosInC.w;
        float2 samplePosInTex;
        samplePosInTex.x = samplePosInNdc.x * 0.5f + 0.5f;
        samplePosInTex.y = 0.5f - samplePosInNdc.y * 0.5f;

        // sample expected depth in view space
        float expectedDepthInV = samplePosInV.z;

        // sample real depth in view space
        float sampleDepth = t_depth.Sample(s_pointClamp, samplePosInTex).r;
        float realDepthInV = ClacNDCDepthToViewDepth(sampleDepth, cb_proj);
        
        // range check
        float depthInVDiff = abs(expectedDepthInV - realDepthInV);

        // if depth difference is too big, it means the sample is far away
        float rangeCheck = smoothstep(0.0f, 1.0f, cb_ssaoRadius / max(depthInVDiff, k_epsilon));

        // occlusion accumulation
        occlusion += (realDepthInV< expectedDepthInV - cb_ssaoBias ? 1.0f : 0.0f) * rangeCheck;
    }

    return 1.0f - (occlusion / max(1.0f, (float) JAM_SSAO_KERNEL_SIZE));
}