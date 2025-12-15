#pragma once

// 1. object 형식 json은 필드 추가, 삭제에 유연하게 대응할 수 있어야 한다.
//    필드가 추가, 삭제 되었다고 해도 throw 하지 않고 정상적으로 직렬화, 역직렬화가 되어야 한다.
//    필드 추가 및 삭제는 로깅 후 무시 or 기본값으로 조용히 처리한다.
//    단, 필수 필드가 누락된 경우, 데이터 파싱에 실패한 경우에는 throw 한다.
// 2. object 외의 형식의 실패는 대부분 데이터 파싱 실패이므로 throw 한다.


namespace jam
{

class ImageTexture;
class Material;
class Camera;

}

namespace nlohmann
{

using Json = jam::Json;
using namespace jam;

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

void to_json(Json& _outJson, const Camera& _camera);
void from_json(const Json& _json, Camera& _outCamera);

template<typename E>
    requires std::is_enum_v<E>
void to_json(Json& _outJson, E _enum)
{
    _outJson = ErrorMessageFrom(_enum);
}

template<typename E>
void from_json(const Json& _json, E& _outEnum)
    requires std::is_enum_v<E>
{
    const EnumIntType<E> strValue = _json.get<std::string>();   // or throw
    if (const std::optional<E> opt = EnumFrom<E>(strValue))
    {
        _outEnum = opt.value();
    }
    else
    {
        throw std::runtime_error(std::format("invalid enum value: {}", strValue));
    }
}

}   // namespace nlohmann