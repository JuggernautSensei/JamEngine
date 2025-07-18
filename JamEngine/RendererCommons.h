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
    Immutable,      // read only by GPU                     -> immutable
    GPUWriteable,   // read/write by GPU                    -> default
    CPUReadable,    // read by CPU and read/write by GPU    -> staging
    CPUWriteable,   // write by CPU and read/write by GPU   -> dynamic
};

}   // namespace jam