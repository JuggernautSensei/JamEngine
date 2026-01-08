#include "pch.h"

#include "Json.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"

namespace jam
{

Optional<Json> LoadJsonFile(
    const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    try
    {
        return Json::parse(ifs);
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to parse json file: {}. {}", _path.native(), e.what());
        return k_nullopt;
    }
}

bool SaveJsonFile(
    const fs::path& _path,
    const Json&     _json,
    const Int32     _dump)
{
    std::ofstream ofs { _path, std::ios::binary };
    if (!ofs)
    {
        GlobalBuffer::SetLastError("failed to open file for writing: {}", _path.native());
        return false;
    }

    try
    {
        if (_dump < 0)
        {
            ofs << _json;
        }
        else
        {
            ofs << _json.dump(_dump);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to write json file: {}. {}", _path.native(), e.what());
        return false;
    }
}

Optional<Json> LoadBsonFile(
    const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    try
    {
        return Json::from_bson(ifs);
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to parse bson file: {}. {}", _path.native(), e.what());
        return k_nullopt;
    }
}

bool SaveBsonFile(
    const fs::path& _path,
    const Json&     _j)
{
    std::vector<std::uint8_t> bytes = Json::to_bson(_j);
    return SaveBinaryFile(_path, bytes);
}

}   // namespace jam

namespace nlohmann
{

void to_json(
    Json&            _outJson,
    const StringView _str)
{
    _outJson = _str.ToString();
}

void to_json(
    Json&           _outJson,
    const fs::path& _path)
{
    _outJson = _path.native();
}

void from_json(
    const Json& _json,
    fs::path&   _outPath)
{
    _outPath = _json.get<std::string>();
}

void to_json(
    Json& _outJson,
    Vec3  _vec3)
{
    _outJson = Json::array({ _vec3.x, _vec3.y, _vec3.z });
}

void from_json(
    const Json& _json,
    Vec3&       _outVec3)
{
    _outVec3.x = _json.at(0).get<float>();
    _outVec3.y = _json.at(1).get<float>();
    _outVec3.z = _json.at(2).get<float>();
}

void to_json(
    Json& _outJson,
    Vec2  _vec2)
{
    _outJson = Json::array({ _vec2.x, _vec2.y });
}

void from_json(
    const Json& _json,
    Vec2&       _outVec2)
{
    _outVec2.x = _json.at(0).get<float>();
    _outVec2.y = _json.at(1).get<float>();
}

void to_json(
    Json& _outJson,
    Quat  _q)
{
    _outJson = Json::array({ _q.x, _q.y, _q.z, _q.w });
}

void from_json(
    const Json& _json,
    Quat&       _outQ)
{
    _outQ.x = _json.at(0).get<float>();
    _outQ.y = _json.at(1).get<float>();
    _outQ.z = _json.at(2).get<float>();
    _outQ.w = _json.at(3).get<float>();
}

void to_json(
    Json&       _outJson,
    const Mat4& _mat4)
{
    _outJson = Json::array(
        { _mat4._11,
          _mat4._12,
          _mat4._13,
          _mat4._14,
          _mat4._21,
          _mat4._22,
          _mat4._23,
          _mat4._24,
          _mat4._31,
          _mat4._32,
          _mat4._33,
          _mat4._34,
          _mat4._41,
          _mat4._42,
          _mat4._43,
          _mat4._44 });
}

void from_json(
    const Json& _json,
    Mat4&       _outMat4)
{
    _outMat4._11 = _json.at(0).get<float>();
    _outMat4._12 = _json.at(1).get<float>();
    _outMat4._13 = _json.at(2).get<float>();
    _outMat4._14 = _json.at(3).get<float>();
    _outMat4._21 = _json.at(4).get<float>();
    _outMat4._22 = _json.at(5).get<float>();
    _outMat4._23 = _json.at(6).get<float>();
    _outMat4._24 = _json.at(7).get<float>();
    _outMat4._31 = _json.at(8).get<float>();
    _outMat4._32 = _json.at(9).get<float>();
    _outMat4._33 = _json.at(10).get<float>();
    _outMat4._34 = _json.at(11).get<float>();
    _outMat4._41 = _json.at(12).get<float>();
    _outMat4._42 = _json.at(13).get<float>();
    _outMat4._43 = _json.at(14).get<float>();
    _outMat4._44 = _json.at(15).get<float>();
}

void to_json(
    Json&            _outJson,
    const Ref<Mesh>& _pMesh)
{
    if (_pMesh && _pMesh->GetPath().empty() == false)
    {
        _outJson = _pMesh->GetPath();
    }
    else
    {
        _outJson = nullptr;
    }
}

void from_json(
    const Json& _json,
    Ref<Mesh>&  _pOutMesh)
{
    if (_json.is_null())
    {
        _pOutMesh = nullptr;
    }
    else
    {
        const fs::path path = _json.get<fs::path>();
        if (!MoveIfHasValue(AssetManager::LoadMesh(path), _pOutMesh))
        {
            throw std::runtime_error(std::format("failed to load Mesh from path: {}. {}", path.native(), GlobalBuffer::GetLastError()));
        }
    }
}

void to_json(
    Json&                _outJson,
    const Ref<Material>& _pMaterial)
{
    if (_pMaterial && _pMaterial->GetPath().empty() == false)
    {
        _outJson = _pMaterial->GetPath();
    }
    else
    {
        _outJson = nullptr;
    }
}

void from_json(
    const Json&    _json,
    Ref<Material>& _pOutMaterial)
{
    if (_json.is_null())
    {
        _pOutMaterial = nullptr;
    }
    else
    {
        const fs::path path = _json.get<fs::path>();
        if (!MoveIfHasValue(AssetManager::LoadMaterial(path), _pOutMaterial))
        {
            throw std::runtime_error(std::format("failed to load Material from path: {}. {}", path.native(), GlobalBuffer::GetLastError()));
        }
    }
}

void to_json(
    Json&                    _outJson,
    const Ref<ImageTexture>& _pImageTexture)
{
    if (_pImageTexture && _pImageTexture->GetPath().empty() == false)
    {
        _outJson = _pImageTexture->GetPath();
    }
    else
    {
        _outJson = nullptr;
    }
}

void from_json(
    const Json&        _json,
    Ref<ImageTexture>& _pOutImageTexture)
{
    if (_json.is_null())
    {
        _pOutImageTexture = nullptr;
    }
    else
    {
        const fs::path path = _json.get<fs::path>();
        if (!MoveIfHasValue(AssetManager::LoadImageTexture(path), _pOutImageTexture))
        {
            throw std::runtime_error(std::format("failed to load ImageTexture from path: {}. {}", path.native(), GlobalBuffer::GetLastError()));
        }
    }
}

}   // namespace nlohmann