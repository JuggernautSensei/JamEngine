#pragma once

#include <cista/mmap.h>
#include <cista/serialization.h>

#include "File.h"
#include "FileSystem.h"

namespace Jam
{
struct Aabb;

using AssetMMAP    = cista::mmap;
using AssetMMAPBuf = cista::buf<cista::mmap>;

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

                case eFileWriterCollisionPolicy::Overwrite:
                case eFileWriterCollisionPolicy::Append:
                default:
                    JAM_UNREACHABLE;
            }
        }

        try
        {
            return AssetSaver { AssetMMAPBuf { AssetMMAP { _path.GetPtr(), cista::mmap::protection::WRITE } } };
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
        AssetMMAPBuf&& _buf)
        : m_mmioBuf(std::move(_buf))
    {
    }

    AssetMMAPBuf m_mmioBuf = {};
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
            return AssetLoader { AssetMMAPBuf { AssetMMAP { _path.GetPtr(), cista::mmap::protection::READ } } };
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
        AssetMMAPBuf&& _buf)
        : m_mmioBuf(std::move(_buf))
    {
    }

    AssetMMAPBuf m_mmioBuf = {};
};

namespace Dto
{
    // types
    using StringView = cista::offset::string_view;

    template<typename T>
    using Vector = cista::offset::vector<T>;

    template<typename T, size_t N>
    using Array = cista::offset::array<T, N>;

    struct Vec3
    {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
    };

    struct Matrix
    {
        Array<float, 16> m = {};
    };

    struct Aabb
    {
        Vec3 center  = {};
        Vec3 extents = {};
    };

    // wrap/unwrap
    [[nodiscard]] StringView      Wrap(Jam::StringView _str);
    [[nodiscard]] Jam::StringView Unwrap(StringView _str);

    [[nodiscard]] Vec3      Wrap(const Jam::Vec3& _vec);
    [[nodiscard]] Jam::Vec3 Unwrap(const Vec3& _vec);

    [[nodiscard]] Matrix    Wrap(const Jam::Mtx4& _mtx);
    [[nodiscard]] Jam::Mtx4 Unwrap(const Matrix& _mtx);

    [[nodiscard]] Aabb      Wrap(const Jam::Aabb& _aabb);
    [[nodiscard]] Jam::Aabb Unwrap(const Aabb& _aabb);

    // container wrap/unwrap helper

    template<typename TDto, typename TPrimtive>
    [[nodiscard]] Vector<TDto> Wrap(
        const Jam::Vector<TPrimtive>& _vec)
    {
        Vector<TDto> result = {};
        result.reserve(static_cast<UInt32>(_vec.size()));
        for (const auto& item: _vec)
        {
            if constexpr (cista::to_tuple_works_v<TPrimtive>)
            {
                static_assert(std::is_same_v<TPrimtive, TDto>, "TPrimtive and TDto must be the same if TPrimtive is directly serializable by cista.");
                result.emplace_back(Wrap(item));
            }
            else
            {
                result.emplace_back(item);
            }
        }
        return result;
    }

    template<typename TPrimtive, typename TDto>
    [[nodiscard]] Jam::Vector<TPrimtive> Unwrap(
        const Vector<TDto>& _vec)
    {
        Jam::Vector<TPrimtive> result = {};
        result.reserve(_vec.size());
        for (const TPrimtive& item: _vec)
        {
            if constexpr (cista::to_tuple_works_v<TPrimtive>)
            {
                static_assert(std::is_same_v<TPrimtive, TDto>, "TPrimtive and TDto must be the same if TPrimtive is directly serializable by cista.");
                result.emplace_back(Unwrap(item));
            }
            else
            {
                result.emplace_back(item);
            }
        }
        return result;
    }

    template<typename TDto, typename TPrimtive, size_t N>
    [[nodiscard]] Array<TDto, N> Wrap(
        const Jam::Array<TPrimtive, N>& _arr)
    {
        Array<TDto, N> result = {};
        for (size_t i = 0; i < N; ++i)
        {
            if constexpr (cista::to_tuple_works_v<TPrimtive>)
            {
                static_assert(std::is_same_v<TPrimtive, TDto>, "TPrimtive and TDto must be the same if TPrimtive is directly serializable by cista.");
                result[i] = Wrap(_arr[i]);
            }
            else
            {
                result[i] = _arr[i];
            }
        }
        return result;
    }

    template<typename TPrimtive, typename TDto, size_t N>
    [[nodiscard]] Jam::Array<TPrimtive, N> Unwrap(
        const Array<TDto, N>& _arr)
    {
        Jam::Array<TPrimtive, N> result;
        for (size_t i = 0; i < N; ++i)
        {
            if constexpr (cista::to_tuple_works_v<TPrimtive>)
            {
                static_assert(std::is_same_v<TPrimtive, TDto>, "TPrimtive and TDto must be the same if TPrimtive is directly serializable by cista.");
                result[i] = Unwrap(_arr[i]);
            }
            else
            {
                result[i] = _arr[i];
            }
        }
        return result;
    }

};   // namespace Dto

}   // namespace Jam
