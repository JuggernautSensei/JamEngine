#pragma once
#include "Asset.h"
#include "Event.h"

namespace jam
{
enum class eAssetType;
}

namespace jam
{

class WindowResizeEvent final : public Event
{
public:
    WindowResizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    NODISCARD Int32 GetWidth() const { return m_width; }
    NODISCARD Int32 GetHeight() const { return m_height; }

    JAM_EVENT(WindowResizeEvent,
              eEventCategoryFlags_Window,
              std::format("WindowResizeEvent: width: {}, height: {}", m_width, m_height))

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMoveEvent final : public Event
{
public:
    WindowMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    NODISCARD Int32 GetX() const { return m_x; }
    NODISCARD Int32 GetY() const { return m_y; }
    JAM_EVENT(WindowMoveEvent,
              eEventCategoryFlags_Window,
              std::format("WindowMoveEvent: x: {}, y: {}", m_x, m_y))
private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent final : public Event
{
public:
    WindowCloseEvent() = default;
    JAM_EVENT(WindowCloseEvent,
              eEventCategoryFlags_Window,
              "WindowCloseEvent: Window is closing")
};

class MouseWheelEvent final : public Event
{
public:
    explicit MouseWheelEvent(const Int32 _delta)
        : m_delta(_delta)
    {
    }

    NODISCARD Int32 GetDelta() const { return m_delta; }
    JAM_EVENT(MouseWheelEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("MouseWheelEvent: delta: {}", m_delta))

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent final : public Event
{
public:
    MouseMoveEvent(const UInt32 _x, const UInt32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }
    NODISCARD UInt32 GetX() const { return m_x; }
    NODISCARD UInt32 GetY() const { return m_y; }
    JAM_EVENT(MouseMoveEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("MouseMoveEvent: x: {}, y: {}", m_x, m_y))
private:
    UInt32 m_x = 0;
    UInt32 m_y = 0;
};

class MouseDownEvent final : public Event
{
public:
    explicit MouseDownEvent(const int _vKey)
        : m_vKey(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_vKey; }
    JAM_EVENT(MouseDownEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("MouseDownEvent: vKey: {}", m_vKey))
private:
    int m_vKey = 0;
};

class MouseUpEvent final : public Event
{
public:
    explicit MouseUpEvent(const int _vKey)
        : m_vKey(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_vKey; }
    JAM_EVENT(MouseUpEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("MouseUpEvent: vKey: {}", m_vKey))
private:
    int m_vKey = 0;
};

class KeyDownEvent final : public Event
{
public:
    explicit KeyDownEvent(const int _vKey)
        : m_key(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_key; }
    JAM_EVENT(KeyDownEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("KeyDownEvent: vKey: {}", m_key))
private:
    int m_key = 0;
};

class KeyUpEvent final : public Event
{
public:
    explicit KeyUpEvent(const int _vKey)
        : m_key(_vKey)
    {
    }
    NODISCARD int GetVKey() const { return m_key; }
    JAM_EVENT(KeyUpEvent,
              eEventCategoryFlags_Input | eEventCategoryFlags_Window,
              std::format("KeyUpEvent: vKey: {}", m_key))
private:
    int m_key = 0;
};

class BackBufferCleanupEvent final : public Event
{
public:
    BackBufferCleanupEvent() = default;
    JAM_EVENT(BackBufferCleanupEvent,
              eEventCategoryFlags_Renderer,
              "BackBufferCleanupEvent: Cleaning up back buffer")
};

class FileAddEvent final : public Event
{
public:
    explicit FileAddEvent(const fs::path& _path)
        : m_path(_path.string())
    {
    }
    NODISCARD std::string_view GetPath() const { return m_path; }
    JAM_EVENT(FileAddEvent,
              eEventCategoryFlags_FileSystem,
              std::format("FileAddEvent: asset: {}", m_path))
private:
    std::string m_path;
};

class FileRemoveEvent final : public Event
{
public:
    explicit FileRemoveEvent(const fs::path& _path)
        : m_path(_path.string())
    {
    }
    NODISCARD std::string_view GetPath() const { return m_path; }
    JAM_EVENT(FileRemoveEvent,
              eEventCategoryFlags_FileSystem,
              std::format("FileRemoveEvent: asset: {}", m_path))
private:
    std::string m_path;
};

class FileModifiedEvent final : public Event
{
public:
    explicit FileModifiedEvent(const fs::path& _path)
        : m_path(_path.string())
    {
    }
    NODISCARD std::string_view GetPath() const { return m_path; }
    JAM_EVENT(FileModifiedEvent,
              eEventCategoryFlags_FileSystem,
              std::format("FileModifiedEvent: asset: {}", m_path))
private:
    std::string m_path;
};

class FileRenamedEvent final : public Event
{
public:
    FileRenamedEvent(const fs::path& _oldPath, const fs::path& _newPath)
        : m_oldPath(_oldPath.string())
        , m_newPath(_newPath.string())
    {
    }
    NODISCARD std::string_view GetOldPath() const { return m_oldPath; }
    NODISCARD std::string_view GetNewPath() const { return m_newPath; }
    JAM_EVENT(FileRenamedEvent,
              eEventCategoryFlags_FileSystem,
              std::format("FileRenamedEvent: old asset: {}, new asset: {}", m_oldPath, m_newPath))
private:
    std::string m_oldPath;
    std::string m_newPath;
};

class SceneChangeEvent final : public Event
{
public:
    explicit SceneChangeEvent(std::string _sceneName)
        : m_sceneName(std::move(_sceneName))
    {
    }

    NODISCARD std::string_view GetSceneName() const { return m_sceneName; }
    JAM_EVENT(SceneChangeEvent,
              eEventCategoryFlags_SceneLayer,
              std::format("SceneChangeEvent: scene name: {}", m_sceneName))

private:
    std::string m_sceneName;
};

class AssetLoadEvent final : public Event
{
public:
    AssetLoadEvent(const eAssetType _assetType, fs::path _assetPath)
        : m_assetType(_assetType)
        , m_assetPath(std::move(_assetPath))
    {
    }
    NODISCARD eAssetType GetAssetType() const { return m_assetType; }
    NODISCARD const fs::path& GetAssetPath() const { return m_assetPath; }
    JAM_EVENT(AssetLoadEvent,
              eEventCategoryFlags_AssetManager,
              std::format("AssetLoadEvent: asset type: {}, asset asset: {}", EnumToString(m_assetType), m_assetPath.string()))

private:
    eAssetType m_assetType;   // default asset type, can be changed later
    fs::path   m_assetPath;   // asset to the asset file
};

class AssetUnloadEvent final : public Event
{
public:
    explicit AssetUnloadEvent(const eAssetType _assetType, fs::path _assetPath)
        : m_assetType(_assetType)
        , m_assetPath(std::move(_assetPath))
    {
    }
    NODISCARD eAssetType GetAssetType() const { return m_assetType; }
    NODISCARD const fs::path& GetAssetPath() const { return m_assetPath; }
    JAM_EVENT(AssetUnloadEvent,
              eEventCategoryFlags_AssetManager,
              std::format("AssetUnloadEvent: asset type: {}, asset asset: {}", EnumToString(m_assetType), m_assetPath.string()))

private:
    eAssetType m_assetType;   // default asset type, can be changed later
    fs::path   m_assetPath;   // asset to the asset file
};

class AssetModifiedEvent final : public Event
{
public:
    explicit AssetModifiedEvent(const eAssetType _assetType, fs::path _assetPath)
        : m_assetType(_assetType)
        , m_assetPath(std::move(_assetPath))
    {
    }
    NODISCARD eAssetType GetAssetType() const { return m_assetType; }
    NODISCARD const fs::path& GetAssetPath() const { return m_assetPath; }
    JAM_EVENT(AssetModifiedEvent,
              eEventCategoryFlags_AssetManager,
              std::format("AssetModifiedEvent: asset type: {}, asset asset: {}", EnumToString(m_assetType), m_assetPath.string()))

private:
    eAssetType m_assetType;   // default asset type, can be changed later
    fs::path   m_assetPath;   // asset to the asset file
};

}   // namespace jam