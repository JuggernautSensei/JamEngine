#pragma once
#include "HandleAllocator.h"
#include "Version.h"

namespace jam
{

enum class eAssetType : Int8
{
    Unknown = -1,

    ImageTexture = 0,
    Material,
    Mesh,
    // Audio,
    // Font
};

class IAsset
{
public:
    IAsset()          = default;
    virtual ~IAsset() = default;

    IAsset(const IAsset&)            = default;
    IAsset(IAsset&&)                 = default;
    IAsset& operator=(const IAsset&) = default;
    IAsset& operator=(IAsset&&)      = default;

    virtual eErrorCode Load(const fs::path& _path)       = 0;
    virtual void       Unload()                          = 0;
    virtual eErrorCode Save(const fs::path& _path) const = 0;

    JAM_NODISCARD virtual bool IsLoaded() const = 0;

    JAM_NODISCARD virtual const fs::path&  GetPath() const = 0;
    JAM_NODISCARD virtual eAssetType       GetType() const = 0;
    JAM_NODISCARD virtual UInt32           GetHash() const = 0;
    JAM_NODISCARD virtual std::string_view GetName() const = 0;
};

}   // namespace jam
