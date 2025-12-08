#pragma once
#include "AssetManager.h"
#include "IAsset.h"
#include "Script.h"
#include "ScriptDB.h"

namespace nlohmann
{

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Quat, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Mat4, _11, _12, _13, _14, _21, _22, _23, _24, _31, _32, _33, _34, _41, _42, _43, _44);

}   // namespace nlohmann

namespace jam
{

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] E GetEnumFromJsonOrThrow(const Json& _json)
{
    const E e = _json.get<E>();
    if (IsValidEnum(e))
    {
        return e;
    }
    else
    {
        throw std::exception { "Invalid enum value in JSON." };
    }
}

template<typename TKey, typename E>
    requires std::is_enum_v<E>
[[nodiscard]] E ValueEnumFromJson(const Json& _json, TKey&& _key, E&& _defaultE)
{
    const E e = _json.value<E>(_key, _defaultE);
    if (IsValidEnum(e))
    {
        return e;
    }
    else
    {
        return _defaultE;
    }
}

[[nodiscard]] Json AssetToJson(const Ref<IAsset>& _pAssetOrNull);

template<typename T>
    requires std::is_base_of_v<IAsset, T>
[[nodiscard]] Ref<T> AssetFromJsonOrThrow(const Json& _json)
{
    if (_json.is_null())
    {
        return nullptr;
    }
    else
    {
        const fs::path path = _json.get<std::string>();
        if (std::optional<Ref<IAsset>> opt = AssetManager::GetOrLoad(T::k_type, path))
        {
            Ref<IAsset> pAsset = opt.value();
            return pAsset.As<T>();
        }
        else
        {
            throw std::exception { GetErrorString() };
        }
    }
}

}   // namespace jam
