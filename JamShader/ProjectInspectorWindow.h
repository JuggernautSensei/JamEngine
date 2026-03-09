#pragma once

class ProjectChangedEvent;

class ProjectInspectorWindow
{
public:
    void OnImGuiRender();
    void OnEvent(Event& _event);

private:
    void OnProjectChangedEvent_(const ProjectChangedEvent& _event);

    void Widget_ProjectName_();

    // project operator
    void NewShaderItem_();
    void RemoveFocisingShaderItem_();
    void CloneFocisingShaderItem_();
    void ClearShaderItems_() const;

    void SetFocusingShaderItem_(StringView _nameOrEmpty);

    // project
    Project* m_pProject          = nullptr;
    String   m_projectRenameEdit = {};

    // shader item
    String            m_focusingShaderItemName = {};
    const ShaderItem* m_pFocusingShaderItem    = nullptr;
};
