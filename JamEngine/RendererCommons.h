#pragma once

namespace jam
{

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
    Immutable    = D3D11_USAGE_IMMUTABLE,   // read only by GPU                     -> immutable
    GPUWriteable = D3D11_USAGE_DEFAULT,     // read/write by GPU                    -> default
    CPUReadable  = D3D11_USAGE_STAGING,     // read by CPU and read/write by GPU    -> staging
    CPUWriteable = D3D11_USAGE_DYNAMIC,     // write by CPU and read/write by GPU   -> dynamic
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