#pragma once
#include "File.h"

namespace Jam
{

enum class eAssetType
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

    virtual Error Load(const Path& _path)                                                                                            = 0;
    virtual Error Save(const Path& _path, bool _bReplaceExtension = true, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None) const = 0;
    virtual void  Invalidate()                                                                                                       = 0;
    virtual void  SetName(StringView _name)                                                                                          = 0;
    virtual void  SetVirtualPath(const Path& _path)                                                                                  = 0;

    [[nodiscard]] virtual bool        IsValid() const      = 0;
    [[nodiscard]] virtual eAssetType  GetAssetType() const = 0;
    [[nodiscard]] virtual StringView  GetTypeName() const  = 0;
    [[nodiscard]] virtual UInt32      GetTypeHash() const  = 0;
    [[nodiscard]] virtual const Path& GetPath() const      = 0;
    [[nodiscard]] virtual StringView  GetName() const      = 0;
    [[nodiscard]] virtual StringView  GetExtension() const = 0;
    [[nodiscard]] virtual StringView  GetFilter() const    = 0;

protected:
    IAsset() = default;
};

// path is the file path of the asset & this will be used as the unique identifier of the asset
// typeName is a user-defined typeName for the asset & this is optional and can be empty
// path and typeName can be different
// path is more like the "ID" of the asset, while typeName is more like a "label" for easier identification
// typeName is just for user convenience and has no effect on asset management or loading/unloading.
// SetVirtualPath can be used to change the path of an asset that is not loaded from a file

}   // namespace Jam
