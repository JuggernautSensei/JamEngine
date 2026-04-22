#pragma once
#include "Textures.h"

namespace jam
{
class EditorLayer;

class ToolbarPanel
{
public:
    explicit ToolbarPanel(EditorLayer* _pEditorLayer);
    void Show(bool* _pOpen) const;

private:
    void DrawPlayButton_(const ImVec2& _imageSize) const;
    void DrawPauseButton_(const ImVec2& _imageSize) const;
    void DrawSimulateButton_(const ImVec2& _imageSize) const;
    void DrawResetButton_(const ImVec2& _imageSize) const;
    void DrawStepButton(const ImVec2& _imageSize) const;

    EditorLayer* m_pEditorLayer = nullptr;

    TextureRef m_playIcon;       // ▶
    TextureRef m_pauseICon;       // ∥
    TextureRef m_resetIcon;      // ■
    TextureRef m_simulateIcon;   // ▶|
};

}   // namespace jam
