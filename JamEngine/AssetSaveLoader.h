#pragma once

#include <cista/mmap.h>
#include <cista/serialization.h>

#include "File.h"
#include "FileSystem.h"

namespace Jam
{

using AssetMMIO       = cista::mmap;
using AssetMMIOBuffer = cista::buf<cista::mmap>;
using eAssetRWFlags   = cista::mode;

template<typename TDto>
class AssetSaver
{
public:
    static Expect<AssetSaver> Open(
        const StringView                 _path,
        const eFileWriterCollisionPolicy _policy)
    {
        JAM_ASSERT(_policy != eFileWriterCollisionPolicy::Append, "WriteElement mode is not supported for AssetSaver.");

        if (_policy != eFileWriterCollisionPolicy::Overwrite && FileSystem::Exists(_path))
        {
            switch (_policy)
            {
                case eFileWriterCollisionPolicy::Rename:
                {
                    return Open(FileSystem::GenerateUniquePath(_path), eFileWriterCollisionPolicy::Overwrite);
                }

                case eFileWriterCollisionPolicy::Fail:
                {
                    JAM_LOG_ERROR("File '{}' already exists. Open failed due to collision policy.", _path);
                    return Error::AlreadyExists;
                }

                default:
                    JAM_UNREACHABLE;
            }
        }

        try
        {
            return AssetSaver { AssetMMIOBuffer { AssetMMIO { _path.GetPtr(), cista::mmap::protection::WRITE } } };
        }
        catch (const std::exception& e)
        {
            JAM_LOG_ERROR("Failed to open AssetLoader for file '{}': {}", _path, e.what());
            return Error::OpenFailed;
        }
    }

    Error Write(TDto& _obj)
    {
        try
        {
            cista::serialize(m_mmioBuf, _obj);
            return Error::None;
        }
        catch (const std::exception& e)
        {
            JAM_LOG_ERROR("Failed to serialize cista object: {}", e.what());
            return Error::ParseError;
        }
    }

private:
    explicit AssetSaver(
        AssetMMIOBuffer&& _buf)
        : m_mmioBuf(std::move(_buf))
    {
    }

    AssetMMIOBuffer m_mmioBuf = {};
};

template<typename TDto>
class AssetLoader
{
public:
    static Expect<AssetLoader> Open(
        StringView _path)
    {
        try
        {
            return AssetLoader { AssetMMIOBuffer { AssetMMIO { _path.GetPtr(), cista::mmap::protection::READ } } };
        }
        catch (const std::exception& e)
        {
            JAM_LOG_ERROR("Failed to open AssetLoader for file '{}': {}", _path, e.what());
            return Error::OpenFailed;
        }
    }

    [[nodiscard]] Expect<TDto> Read()
    {
        try
        {
            return *cista::deserialize<TDto>(m_mmioBuf);   // need check..
        }
        catch (const std::exception& e)
        {
            JAM_LOG_ERROR("Failed to deserialize cista object: {}", e.what());
            return Error::ParseError;
        }
    }

private:
    explicit AssetLoader(
        AssetMMIOBuffer&& _buf)
        : m_mmioBuf(std::move(_buf))
    {
    }

    AssetMMIOBuffer m_mmioBuf = {};
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Aabb;

template<typename T, size_t N>
using ArrayDto = cista::offset::array<T, N>;

template<typename T, size_t N>
[[nodiscard]] ArrayDto<T, N> ArrayDtoFrom(const Array<T, N>& _arr)
{
    ArrayDto<T, N> dto = {};
    for (size_t i = 0; i < N; ++i)
    {
        dto[i] = _arr[i];
    }
    return dto;
}

template<typename T, size_t N>
[[nodiscard]] Array<T, N> ArrayFrom(const ArrayDto<T, N>& _dto)
{
    Array<T, N> arr = {};
    for (size_t i = 0; i < N; ++i)
    {
        arr[i] = _dto[i];
    }
    return arr;
}

template<typename T>
using VectorDto = cista::offset::vector<T>;

template<typename T>
using UniquePtrDto = cista::offset::unique_ptr<T>;

template<typename T>
[[nodiscard]] VectorDto<T> VectorDtoFrom(const Vector<T>& _vec)
{
    return cista::offset::to_vec(_vec);
}

template<typename T>
[[nodiscard]] Vector<T> VectorFrom(const VectorDto<T>& _dto)
{
    Vector<T> vec = {};
    vec.reserve(_dto.size());
    for (const T& item: _dto)
    {
        vec.emplace_back(item);
    }
    return vec;
}

// string
using StringViewDto = cista::offset::string_view;
[[nodiscard]] String        StringFrom(StringViewDto _str);
[[nodiscard]] StringView    StringViewFrom(StringViewDto _str);
[[nodiscard]] StringViewDto StringViewDtoFrom(StringView _str);

struct Vec3Dto
{
    [[nodiscard]] static Vec3Dto FromVec3(const Vec3& _v);
    [[nodiscard]] Vec3           ToVec3() const;

    ArrayDto<float, 3> vec = {};
};

struct Mtx4Dto
{
    [[nodiscard]] static Mtx4Dto FromMat4(Mtx4 _mtx);
    [[nodiscard]] Mtx4           ToMtx4() const;

    ArrayDto<float, 16> mtx = {};
};

struct AabbDto
{
    [[nodiscard]] static AabbDto FromAabb(const Aabb& _aabb);
    [[nodiscard]] Aabb           ToAabb() const;

    Vec3Dto min = {};
    Vec3Dto max = {};
};

}   // namespace Jam
