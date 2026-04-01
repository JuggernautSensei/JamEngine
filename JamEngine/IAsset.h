#pragma once
#include "File.h"
#include "Signal.h"

namespace Jam
{

enum class eAsset
{
    Texture = 0,
    Material,
    Mesh,
    Model,
};

class IAsset : public RefCounted
{
public:
    ~IAsset() override = default;

    IAsset(const IAsset&)            = default;
    IAsset(IAsset&&)                 = default;
    IAsset& operator=(const IAsset&) = default;
    IAsset& operator=(IAsset&&)      = default;

    virtual Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const = 0;
    virtual void  Invalidate()                                                                                                                             = 0;
    virtual void  SetName(StringView _name)                                                                                                                = 0;
    virtual void  SetVirtualPath(StringView _path)                                                                                                         = 0;

    [[nodiscard]] virtual bool       IsValid() const      = 0;
    [[nodiscard]] virtual eAsset GetAssetType() const = 0;
    [[nodiscard]] virtual StringView GetTypeName() const  = 0;
    [[nodiscard]] virtual UInt32     GetTypeHash() const  = 0;
    [[nodiscard]] virtual StringView GetPath() const      = 0;
    [[nodiscard]] virtual StringView GetName() const      = 0;
    [[nodiscard]] virtual StringView GetExtension() const = 0;
    [[nodiscard]] virtual StringView GetFilter() const    = 0;

protected:
    IAsset()                              = default;
    virtual Error Load_(StringView _path) = 0;
    friend class AssetManager;
};

// dir is the file dir of the asset & this will be used as the unique identifier of the asset
// typeName is a user-defined typeName for the asset & this is optional and can be empty
// dir and typeName can be different
// dir is more like the "ID" of the asset, while typeName is more like a "label" for easier identification
// typeName is just for user convenience and has no effect on asset management or loading/unloading.
// SetVirtualPath can be used to change the dir of an asset that is not loaded from a file

}   // namespace Jam
