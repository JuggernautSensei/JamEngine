#pragma once
#include "Camera.h"
#include "Transform.h"

namespace jam
{
class EditorLayer;

class BackBufferCleanupEvent;
class WindowMaximizeEvent;
class WindowResizeEvent;
class Event;

class ViewportPanel
{
public:
    explicit ViewportPanel(EditorLayer* _pEditorLayer);
    void Show(bool* _pOpen);

private:
    struct EditorCamera
    {
        Camera    camera;
        Transform transform;
        float     moveSpeed   = 1.f;
        float     rotateSpeed = 0.03f;
    };

    EditorCamera m_editorCamera;
    EditorLayer* m_pEditorLayer = nullptr;
};

}   // namespace jam