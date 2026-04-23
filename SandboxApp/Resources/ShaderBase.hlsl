#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define PADDING1 float CONCAT(padding1, __LINE__)
#define PADDING2 float2 CONCAT(padding2, __LINE__)
#define PADDING3 float3 CONCAT(padding3, __LINE__)
