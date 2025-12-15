#pragma once
#include "GraphicsBase.h"

namespace jam
{

[[nodiscard]] constexpr ImVec2 operator+(const ImVec2& _lhs, const ImVec2& _rhs)
{
    return ImVec2 { _lhs.x + _rhs.x, _lhs.y + _rhs.y };
}

[[nodiscard]] constexpr ImVec2 operator-(const ImVec2& _lhs, const ImVec2& _rhs)
{
    return ImVec2 { _lhs.x - _rhs.x, _lhs.y - _rhs.y };
}

[[nodiscard]] constexpr ImVec2 operator*(const ImVec2& _lhs, const float _rhs)
{
    return ImVec2 { _lhs.x * _rhs, _lhs.y * _rhs };
}

[[nodiscard]] constexpr ImVec2 operator/(const ImVec2& _lhs, const float _rhs)
{
    return ImVec2 { _lhs.x / _rhs, _lhs.y / _rhs };
}

[[nodiscard]] constexpr ImVec2& operator+=(ImVec2& _lhs, const ImVec2& _rhs)
{
    _lhs.x += _rhs.x;
    _lhs.y += _rhs.y;
    return _lhs;
}

[[nodiscard]] constexpr ImVec2& operator-=(ImVec2& _lhs, const ImVec2& _rhs)
{
    _lhs.x -= _rhs.x;
    _lhs.y -= _rhs.y;
    return _lhs;
}

[[nodiscard]] constexpr ImVec2& operator*=(ImVec2& _lhs, const float _rhs)
{
    _lhs.x *= _rhs;
    _lhs.y *= _rhs;
    return _lhs;
}

[[nodiscard]] constexpr ImVec2& operator/=(ImVec2& _lhs, const float _rhs)
{
    _lhs.x /= _rhs;
    _lhs.y /= _rhs;
    return _lhs;
}

[[nodiscard]] constexpr ImVec4 ImVec4From(const ImVec2& _vec2, const float _z = 0.f, const float _w = 0.f)
{
    return ImVec4 { _vec2.x, _vec2.y, _z, _w };
}

[[nodiscard]] constexpr ImVec2 ImVec2From(const ImVec4& _vec4)
{
    return ImVec2 { _vec4.x, _vec4.y };
}

[[nodiscard]] constexpr ImVec4 operator+(const ImVec4& _lhs, const ImVec4& _rhs)
{
    return ImVec4 { _lhs.x + _rhs.x, _lhs.y + _rhs.y, _lhs.z + _rhs.z, _lhs.w + _rhs.w };
}

[[nodiscard]] constexpr ImVec4 operator-(const ImVec4& _lhs, const ImVec4& _rhs)
{
    return ImVec4 { _lhs.x - _rhs.x, _lhs.y - _rhs.y, _lhs.z - _rhs.z, _lhs.w - _rhs.w };
}

[[nodiscard]] constexpr ImVec4 operator*(const ImVec4& _lhs, const float _rhs)
{
    return ImVec4 { _lhs.x * _rhs, _lhs.y * _rhs, _lhs.z * _rhs, _lhs.w * _rhs };
}

[[nodiscard]] constexpr ImVec4 operator/(const ImVec4& _lhs, const float _rhs)
{
    return ImVec4 { _lhs.x / _rhs, _lhs.y / _rhs, _lhs.z / _rhs, _lhs.w / _rhs };
}

[[nodiscard]] constexpr ImVec4& operator+=(ImVec4& _lhs, const ImVec4& _rhs)
{
    _lhs.x += _rhs.x;
    _lhs.y += _rhs.y;
    _lhs.z += _rhs.z;
    _lhs.w += _rhs.w;
    return _lhs;
}

[[nodiscard]] constexpr ImVec4& operator-=(ImVec4& _lhs, const ImVec4& _rhs)
{
    _lhs.x -= _rhs.x;
    _lhs.y -= _rhs.y;
    _lhs.z -= _rhs.z;
    _lhs.w -= _rhs.w;
    return _lhs;
}

[[nodiscard]] constexpr ImVec4& operator*=(ImVec4& _lhs, const float _rhs)
{
    _lhs.x *= _rhs;
    _lhs.y *= _rhs;
    _lhs.z *= _rhs;
    _lhs.w *= _rhs;
    return _lhs;
}

[[nodiscard]] constexpr ImVec4& operator/=(ImVec4& _lhs, const float _rhs)
{
    _lhs.x /= _rhs;
    _lhs.y /= _rhs;
    _lhs.z /= _rhs;
    _lhs.w /= _rhs;
    return _lhs;
}

[[nodiscard]] constexpr bool operator==(const ImVec2& _lhs, const ImVec2& _rhs)
{
    return _lhs.x == _rhs.x && _lhs.y == _rhs.y;
}

[[nodiscard]] constexpr bool operator!=(const ImVec2& _lhs, const ImVec2& _rhs)
{
    return !(_lhs == _rhs);
}

[[nodiscard]] constexpr bool operator==(const ImVec4& _lhs, const ImVec4& _rhs)
{
    return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z && _lhs.w == _rhs.w;
}

[[nodiscard]] constexpr bool operator!=(const ImVec4& _lhs, const ImVec4& _rhs)
{
    return !(_lhs == _rhs);
}

[[nodiscard]] ImVec4       ImVec4FromHex(UInt32 _rgba);
[[nodiscard]] ImTextureRef ImTextureRefFrom(TextureHandle _texh);
[[nodiscard]] ImVec2       CalcFitButtonSize(Int32 _count = 1, ImVec2 _region = ImVec2 { 0.f, 0.f });
void                       SetNextItemWidthToAvailableWidth();

}   // namespace jam