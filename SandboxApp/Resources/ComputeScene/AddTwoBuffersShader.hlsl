
RWStructuredBuffer<float> b_outBuffer0 : register(u0);
RWStructuredBuffer<float> b_outBuffer1 : register(u1);
RWStructuredBuffer<float> b_outBuffer2 : register(u2);

[numthreads(64, 1, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    b_outBuffer0[_dispatchThreadId.x] = _dispatchThreadId.x;
    b_outBuffer1[_dispatchThreadId.x] = _dispatchThreadId.x * 2.f;
    b_outBuffer2[_dispatchThreadId.x] = b_outBuffer0[_dispatchThreadId.x] + b_outBuffer1[_dispatchThreadId.x];
}