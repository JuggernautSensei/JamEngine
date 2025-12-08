#include "ShaderBase.hlsl"

const static float k_noiseWidth = 4.f;
const static float k_noiseHeight = 4.f;

float PSmain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    // get world normal
    float3 normal = t_gBufferNormal.Sample(s_linearWrap, _input.texCoord0) * 2.f - 1.f;
    normal = normalize(normal);

    // get ndc pos
    float3 posInNdc;
    posInNdc.x = _input.texCoord0.x * 2.f - 1.f;
    posInNdc.y = -(_input.texCoord0.y * 2.f - 1.f);
    posInNdc.z = t_depth.Sample(s_pointClamp, _input.texCoord0).r;

    // world pos
    float4 posInC = mul(float4(posInNdc, 1.0f), cb_viewProjInv);
    float3 posInW = posInC.xyz / posInC.w;

    // noise sampling 
    float2 noiseScale;
    noiseScale.x = cb_screenWidth / k_noiseWidth;
    noiseScale.y = cb_screenHeight / k_noiseHeight;

    // for kernel rotation
    float3 randomVec = normalize(t_ssaoNoise.Sample(s_pointClamp, _input.texCoord0 * noiseScale).xyz);

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
        float3 sampleDirW = mul(kernelDir, TBN); // tangent -> world
        float3 samplePosInW = posInW + sampleDirW * cb_ssaoRadius; // offset position in world

        float4 samplePosInV = mul(float4(samplePosInW, 1.0f), cb_view);
        float4 samplePosInC = mul(samplePosInV, cb_proj);
        float2 samplePosInNdc = samplePosInC.xy / samplePosInC.w;
        float2 samplePosInTex;
        samplePosInTex.x = samplePosInNdc.x * 0.5f + 0.5f;
        samplePosInTex.y = 0.5f - samplePosInNdc.y * 0.5f;

        // sample expected depth in view space
        float expectedDepthInV = samplePosInV.z;

        // sample real depth in view space
        float sampleDepth = t_depth.Sample(s_pointClamp, samplePosInTex).r;
        float realDepthInV = NDCDepthToViewDepth(sampleDepth, cb_proj);
        
        // range check
        float rangeCheck = smoothstep(0.0f, 1.0f, cb_ssaoRadius / abs(expectedDepthInV - realDepthInV));

        // occlusion accumulation
        occlusion += (expectedDepthInV - realDepthInV > cb_ssaoBias) ? rangeCheck : 0.f;
    }

    return 1.0f - (occlusion / max(1.0f, (float) JAM_SSAO_KERNEL_SIZE));
}