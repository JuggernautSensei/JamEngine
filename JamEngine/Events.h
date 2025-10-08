#pragma once
#include "Event.h"

namespace jam
{

class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    JAM_NODISCARD Int32 GetWidth() const
    {
        return m_width;
    }

    JAM_NODISCARD Int32 GetHeight() const
    {
        return m_height;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowResizeEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowResizeEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizeEvent : public Event
{
public:
    WindowMaximizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    JAM_NODISCARD Int32 GetWidth() const
    {
        return m_width;
    }

    JAM_NODISCARD Int32 GetHeight() const
    {
        return m_height;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowMaximizeEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowMaximizeEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizeEvent : public Event
{
public:
    WindowMinimizeEvent() = default;

    JAM_NODISCARD std::string ToString() const override
    {
        return "WindowMinimizeEvent: Window is minimized";
    }

    JAM_EVENT(WindowMinimizeEvent, eEventCategory::Window)
};

class WindowMoveEvent : public Event
{
public:
    WindowMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    JAM_NODISCARD Int32 GetX() const
    {
        return m_x;
    }

    JAM_NODISCARD Int32 GetY() const
    {
        return m_y;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowMoveEvent: x: {}, y: {}", m_x, m_y);
    }

    JAM_EVENT(WindowMoveEvent, eEventCategory::Window)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() = default;

    JAM_NODISCARD std::string ToString() const override
    {
        return "WindowCloseEvent: Window is closing";
    }

    JAM_EVENT(WindowCloseEvent, eEventCategory::Window)
};

class MouseWheelEvent : public Event
{
public:
    explicit MouseWheelEvent(const Int32 _delta)
        : m_delta(_delta)
    {
    }

    JAM_NODISCARD Int32 GetDelta() const
    {
        return m_delta;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseWheelEvent: delta: {}", m_delta);
    }

    JAM_EVENT(MouseWheelEvent, eEventCategory::Input)

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    JAM_NODISCARD Int32 GetX() const
    {
        return m_x;
    }

    JAM_NODISCARD Int32 GetY() const
    {
        return m_y;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseMoveEvent: x: {}, y: {}", m_x, m_y);
    }

    JAM_EVENT(MouseMoveEvent, eEventCategory::Input)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class MouseDownEvent : public Event
{
public:
    explicit MouseDownEvent(const Int32 _vKey)
        : m_vKey(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_vKey;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseDownEvent: vKey: {}", m_vKey);
    }

    JAM_EVENT(MouseDownEvent, eEventCategory::Input)

private:
    Int32 m_vKey = 0;
};

class MouseUpEvent : public Event
{
public:
    explicit MouseUpEvent(const Int32 _vKey)
        : m_vKey(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_vKey;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseUpEvent: vKey: {}", m_vKey);
    }

    JAM_EVENT(MouseUpEvent, eEventCategory::Input)

private:
    Int32 m_vKey = 0;
};

class KeyDownEvent : public Event
{
public:
    explicit KeyDownEvent(const Int32 _vKey)
        : m_key(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_key;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("KeyDownEvent: vKey: {}", m_key);
    }

    JAM_EVENT(KeyDownEvent, eEventCategory::Input)

private:
    Int32 m_key = 0;
};

class KeyUpEvent : public Event
{
public:
    explicit KeyUpEvent(const Int32 _vKey)
        : m_key(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_key;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("KeyUpEvent: vKey: {}", m_key);
    }

    JAM_EVENT(KeyUpEvent, eEventCategory::Input)

private:
    Int32 m_key = 0;
};

class SceneChangeEvent : public Event
{
public:
    explicit SceneChangeEvent(std::string _sceneName)
        : m_sceneName(std::move(_sceneName))
    {
    }

    JAM_NODISCARD std::string_view GetSceneName() const
    {
        return m_sceneName;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("SceneChangeEvent: scene name: {}", m_sceneName);
    }

    JAM_EVENT(SceneChangeEvent, eEventCategory::FileSystem)

private:
    std::string m_sceneName;
};

//
// class FileAddEvent : public Event
//{
// public:
//    explicit FileAddEvent(const fs::path& _path)
//        : m_path(_path.native())
//    {
//    }
//
//    JAM_NODISCARD std::string_view GetPath() const
////    {
//        return m_path;
//    }
//
//    JAM_EVENT(FileAddEvent,
//              eEventCategory::FileSystem,
//              std::format("FileAddEvent: asset: {}", m_path))
//
// private:
//    std::string m_path;
//};
//
// class FileRemoveEvent : public Event
//{
// public:
//    explicit FileRemoveEvent(const fs::path& _path)
//        : m_path(_path.native())
//    {
//    }
//
//    JAM_NODISCARD std::string_view GetPath() const
//    {
//        return m_path;
//    }
//
//    JAM_EVENT(FileRemoveEvent,
//              eEventCategory::FileSystem,
//              std::format("FileRemoveEvent: asset: {}", m_path))
// private:
//    std::string m_path;
//};
//
// class FileModifiedEvent : public Event
//{
// public:
//    explicit FileModifiedEvent(const fs::path& _path)
//        : m_path(_path.native())
//    {
//    }
//
//    JAM_NODISCARD std::string_view GetPath() const
//    {
//        return m_path;
//    }
//
//    JAM_EVENT(FileModifiedEvent,
//              eEventCategory::FileSystem,
//              std::format("FileModifiedEvent: asset: {}", m_path))
// private:
//    std::string m_path;
//};
//
// class FileRenamedEvent : public Event
//{
// public:
//    FileRenamedEvent(const fs::path& _oldPath, const fs::path& _newPath)
//        : m_oldPath(_oldpath.native())
//        , m_newPath(_newpath.native())
//    {
//    }
//
//    JAM_NODISCARD std::string_view GetOldPath() const
//    {
//        return m_oldPath;
//    }
//
//    JAM_NODISCARD std::string_view GetNewPath() const
//    {
//        return m_newPath;
//    }
//
//    JAM_EVENT(FileRenamedEvent,
//              eEventCategory::FileSystem,
//              std::format("FileRenamedEvent: old asset: {}, new asset: {}", m_oldPath, m_newPath))
// private:
//    std::string m_oldPath;
//    std::string m_newPath;
//};

//
// class AssetLoadEvent : public Event
//{
// public:
//    AssetLoadEvent(const eAssetType _assetType, fs::path _assetPath)
//        : m_assetType(_assetType)
//        , m_assetPath(std::move(_assetPath))
//    {
//    }
//
//    JAM_NODISCARD eAssetType GetAssetType() const
//    {
//        return m_assetType;
//    }
//
//    JAM_NODISCARD const fs::path& GetAssetPath() const
//    {
//        return m_assetPath;
//    }
//
//    JAM_EVENT(AssetLoadEvent,
//              eEventCategory::AssetManager,
//              std::format("AssetLoadEvent: asset stage: {}, asset asset: {}", EnumToString(m_assetType), m_assetpath.native()))
//
// private:
//    eAssetType m_assetType;   // default asset stage, can be changed later
//    fs::path   m_assetPath;   // asset to the asset file
//};
//
// class AssetUnloadEvent : public Event
//{
// public:
//    explicit AssetUnloadEvent(const eAssetType _assetType, fs::path _assetPath)
//        : m_assetType(_assetType)
//        , m_assetPath(std::move(_assetPath))
//    {
//    }
//    JAM_NODISCARD eAssetType GetAssetType() const
//    {
//        return m_assetType;
//    }
//
//    JAM_NODISCARD const fs::path& GetAssetPath() const
//    {
//        return m_assetPath;
//    }
//
//    JAM_EVENT(AssetUnloadEvent,
//              eEventCategory::AssetManager,
//              std::format("AssetUnloadEvent: asset stage: {}, asset asset: {}", EnumToString(m_assetType), m_assetpath.native()))
//
// private:
//    eAssetType m_assetType;   // default asset stage, can be changed later
//    fs::path   m_assetPath;   // asset to the asset file
//};
//
// class AssetModifiedEvent : public Event
//{
// public:
//    explicit AssetModifiedEvent(const eAssetType _assetType, fs::path _assetPath)
//        : m_assetType(_assetType)
//        , m_assetPath(std::move(_assetPath))
//    {
//    }
//
//    JAM_NODISCARD eAssetType GetAssetType() const
//    {
//        return m_assetType;
//    }
//
//    JAM_NODISCARD const fs::path& GetAssetPath() const
//    {
//        return m_assetPath;
//    }
//
//    JAM_EVENT(AssetModifiedEvent,
//              eEventCategory::AssetManager,
//              std::format("AssetModifiedEvent: asset stage: {}, asset asset: {}", EnumToString(m_assetType), m_assetpath.native()))
//
// private:
//    eAssetType m_assetType;   // default asset stage, can be changed later
//    fs::path   m_assetPath;   // asset to the asset file
//};

}   // namespace jam