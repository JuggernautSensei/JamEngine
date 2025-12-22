#pragma once
#include "HandleAllocator.h"
#include "RefCounted.h"

namespace jam
{

enum class eAssetType : Int8
{
    Unknown = -1,

    ImageTexture = 0,
    Material,
    Mesh,
    Model,
    // Audio,
    // Font
};

class IAsset : public RefCounted
{
public:
    IAsset()           = default;
    ~IAsset() override = default;

    IAsset(const IAsset&)            = default;
    IAsset(IAsset&&)                 = default;
    IAsset& operator=(const IAsset&) = default;
    IAsset& operator=(IAsset&&)      = default;

    virtual bool Load(const fs::path& _path)       = 0;
    virtual void Unload()                          = 0;
    virtual bool Save(const fs::path& _path) const = 0;

    [[nodiscard]] virtual bool             IsValid() const      = 0;
    [[nodiscard]] virtual eAssetType       GetAssetType() const = 0;
    [[nodiscard]] virtual StringView GetTypeName() const  = 0;
    [[nodiscard]] virtual UInt32           GetTypeHash() const  = 0;
    [[nodiscard]] virtual const fs::path&  GetPath() const      = 0;
};

}   // namespace jam
