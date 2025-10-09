#pragma once

namespace jam
{

class IImporter
{
public:
    IImporter()          = default;
    virtual ~IImporter() = default;

    IImporter(const IImporter&)                = default;
    IImporter& operator=(const IImporter&)     = default;
    IImporter(IImporter&&) noexcept            = default;
    IImporter& operator=(IImporter&&) noexcept = default;

    virtual eErrorCode Import() = 0;   // 설정된 경로 또는 소스 코드로 임포트

    virtual void SetPath(const fs::path& _filePath) = 0;   // 파일 경로 설정
    virtual void Reset()                            = 0;
};

}   // namespace jam