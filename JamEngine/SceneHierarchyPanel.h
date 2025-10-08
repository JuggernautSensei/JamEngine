#pragma once

namespace jam
{

class EditorLayer;

class SceneHierarchyPanel
{
public:
    explicit SceneHierarchyPanel(EditorLayer* _pEditorLayer);
    void Show(bool* _pOpenOrNull) const;

private:
    EditorLayer* m_pEditorLayer = nullptr;   // pointer to the editor layer
};

}   // namespace jam
