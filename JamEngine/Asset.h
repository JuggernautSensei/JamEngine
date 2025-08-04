#pragma once

namespace jam
{

enum class eAssetType
{
    Model = 0,
    Texture
};

class Asset
{
public:
    Asset()          = default;
    virtual ~Asset() = default;

    Asset(Asset&&) noexcept            = default;
    Asset& operator=(Asset&&) noexcept = default;
    Asset(const Asset&)                = delete;
    Asset& operator=(const Asset&)     = delete;

    virtual bool Save(const fs::path& _path) const = 0;
    virtual bool Load(const fs::path& _path)       = 0;
    virtual void Unload()                          = 0;

    NODISCARD virtual bool       IsLoaded() const = 0;
    NODISCARD virtual eAssetType GetType() const  = 0;
    NODISCARD const fs::path& GetPath() const { return m_path; }

protected:
    fs::path m_path = L"";
};

}   // namespace jam
