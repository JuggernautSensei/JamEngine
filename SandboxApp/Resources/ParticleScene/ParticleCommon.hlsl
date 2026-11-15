struct Particle
{
    float3 position;
    float age;
    float3 velocity;
    float lifetime;
    float4 color;
    float angle;
    float angularVelocity;
    uint seed;
    float size;
};

cbuffer CBSimulation : register(b0)
{
    float cb_deltaTime;
    float cb_time;
    float cb_wind;
    float cb_size;
    uint cb_count;
    uint cb_previousCount;
    uint cb_effect;
    uint cb_seed;
    float3 cb_extent;
    float cb_fallSpeed;
};
