#pragma once

namespace jam
{

struct BufferInitData
{
    const void* pData = nullptr;
};

struct IndexBufferInitData
{
    const Index* pData = nullptr;
};

struct Texture2DInitData
{
    const void* pData = nullptr;
    UInt32      pitch = 0;
};

struct ShaderCreateInfo
{
    const void* pBytecode      = nullptr;
    size_t      bytecodeLength = 0;
};

enum class eShader : char
{
    VertexShader,
    PixelShader,
    GeometryShader,
    ComputeShader,
    HullShader,
    DomainShader
};

enum class eResourceAccess : char
{
    GPUWriteable = D3D11_USAGE_DEFAULT,     // read/write by GPU                    -> default
    Immutable    = D3D11_USAGE_IMMUTABLE,   // read only by GPU                     -> immutable
    CPUWriteable = D3D11_USAGE_DYNAMIC,     // write by CPU and read/write by GPU   -> dynamic
    CPUReadable  = D3D11_USAGE_STAGING,     // read by CPU and read/write by GPU    -> staging
};

enum class eTopology : char
{
    Undefined     = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,
    PointList     = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
    LineList      = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
    LineStrip     = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
    TriangleList  = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    TriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

}   // namespace jam