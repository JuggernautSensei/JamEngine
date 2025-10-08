#pragma once

namespace jam
{

struct Version
{
    constexpr Version()
    {
    }

    constexpr explicit Version(const UInt8 _major, const UInt8 _minor, const UInt8 _patch)
        : major(_major)
        , minor(_minor)
        , patch(_patch)
    {
    }

    constexpr explicit Version(const UInt32 _version)
    {
        Unpack(_version);
    }

    constexpr JAM_NODISCARD UInt32 Pack() const
    {
        return (major << 16) | (minor << 8) | patch;
    }

    constexpr JAM_NODISCARD void Unpack(const UInt32 _version)
    {
        major = (_version >> 16) & 0xFF;
        minor = (_version >> 8) & 0xFF;
        patch = _version & 0xFF;
    }

    JAM_NODISCARD std::string ToString() const
    {
        return std::format("{}.{}.{}", major, minor, patch);
    }

    UInt8 major = 0;
    UInt8 minor = 0;
    UInt8 patch = 0;
};

}   // namespace jam