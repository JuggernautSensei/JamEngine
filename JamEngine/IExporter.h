#pragma once

namespace jam
{

class IExporter
{
public:
    IExporter()          = default;
    virtual ~IExporter() = default;

    IExporter(const IExporter&)                = default;
    IExporter& operator=(const IExporter&)     = default;
    IExporter(IExporter&&) noexcept            = default;
    IExporter& operator=(IExporter&&) noexcept = default;

    virtual eErrorCode Export()                           = 0;
    virtual void       SetPath(const fs::path& _filePath) = 0;
    virtual void       Reset()                            = 0;
};

}   // namespace jam