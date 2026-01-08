#pragma once
#include "JsonReader.h"
#include "JsonWriter.h"

namespace jam
{

class Mesh;
class ImageTexture;
class Material;
class Camera;

}   // namespace jam

namespace nlohmann
{

using Json = jam::Json;
using namespace jam;

void to_json(Json& _outJson, StringView _str);
// void from_json(const Json& _json, StringView& _outStr); not supported since StringView does not own memory

void to_json(Json& _outJson, const fs::path& _path);
void from_json(const Json& _json, fs::path& _outPath);

void to_json(Json& _outJson, Vec3 _vec3);
void from_json(const Json& _json, Vec3& _outVec3);

void to_json(Json& _outJson, Vec2 _vec2);
void from_json(const Json& _json, Vec2& _outVec2);

void to_json(Json& _outJson, Quat _q);
void from_json(const Json& _json, Quat& _outQ);

void to_json(Json& _outJson, const Mat4& _mat4);
void from_json(const Json& _json, Mat4& _outMat4);

void to_json(Json& _outJson, const Ref<Mesh>& _pMesh);
void from_json(const Json& _json, Ref<Mesh>& _outMesh);

void to_json(Json& _outJson, const Ref<Material>& _pMaterial);
void from_json(const Json& _json, Ref<Material>& _outMaterial);

void to_json(Json& _outJson, const Ref<ImageTexture>& _pTexture);
void from_json(const Json& _json, Ref<ImageTexture>& _outTexture);

template<typename E>
    requires std::is_enum_v<E>
void to_json(Json& _outJson, E _e)
{
    
}

template<typename E>
void from_json(const Json& _json, E& _outEnum)
    requires std::is_enum_v<E>
{
}

template<typename TValue, size_t N>
void to_json(Json& _outJson, const Array<TValue, N>& _arr)
{
    _outJson = Json::array();
    for (const TValue& val: _arr)
    {
        _outJson.push_back(val);
    }
}

template<typename TValue, size_t N>
void from_json(const Json& _json, Array<TValue, N>& _outArr)
{
    if (_json.size() != N)
    {
        throw std::runtime_error(std::format("invalid array size: expected {}, got {}", N, _json.size()));
    }

    for (size_t i = 0; i < N; ++i)
    {
        _outArr[i] = _json.at(i).get<TValue>();
    }
}

}   // namespace nlohmann