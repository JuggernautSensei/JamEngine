#pragma once

namespace jam
{

class EditorLayer;

class MainMenuBarPanel
{
public:
    explicit MainMenuBarPanel(EditorLayer* _pEditorLayer);
    void Show() const;

private:
    EditorLayer* m_pEditorLayer = nullptr;   // pointer to the editor layer
};

}