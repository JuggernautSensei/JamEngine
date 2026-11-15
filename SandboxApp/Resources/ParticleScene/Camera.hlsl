cbuffer CBCamera : register(b1)
{
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
    float3 cb_cameraRight;
    float cb_padding0;
    float3 cb_cameraUp;
    float cb_padding1;
    float3 cb_eye;
    float cb_padding2;
};
