#pragma once
#include "FileIOUtilities.h"

namespace jam
{

enum class eAssetType
{
    ImageTexture = 0,
    Material,
    Mesh,
    Model,

    Min = ImageTexture,
    Max = Model,
};
JAM_ENUM_STATIC_ASSERT(eAssetType);
JAM_ENABLE_ENUM_ITERATOR(eAssetType);

class IAsset : public RefCounted
{
public:
    IAsset()           = default;
    ~IAsset() override = default;

    IAsset(const IAsset&)            = default;
    IAsset(IAsset&&)                 = default;
    IAsset& operator=(const IAsset&) = default;
    IAsset& operator=(IAsset&&)      = default;

    virtual bool Load(const fs::path& _path)                                                                                    = 0;
    virtual void Unload()                                                                                                       = 0;
    virtual bool Save(const fs::path& _path, bool _bReplaceExtension = true, eSaveFileFlags _flags = eSaveFileFlags_None) const = 0;
    virtual void SetName(StringView _name)                                                                                      = 0;
    virtual void SetVirtualPath(const fs::path& _path)                                                                          = 0;

    [[nodiscard]] virtual bool             IsValid() const      = 0;
    [[nodiscard]] virtual eAssetType       GetAssetType() const = 0;
    [[nodiscard]] virtual StringView       GetTypeName() const  = 0;
    [[nodiscard]] virtual UInt32           GetTypeHash() const  = 0;
    [[nodiscard]] virtual const fs::path&  GetPath() const      = 0;
    [[nodiscard]] virtual const StringView GetName() const      = 0;
};

// path is the file path of the asset & this will be used as the unique identifier of the asset
// name is a user-defined name for the asset & this is optional and can be empty
// path and name can be different
// path is more like the "ID" of the asset, while name is more like a "label" for easier identification
// name is just for user convenience and has no effect on asset management or loading/unloading.
// SetVirtualPath can be used to change the path of an asset that is not loaded from a file

}   // namespace jam
