#pragma once

class FocusingShaderItemChagnedEvent;
class ProjectChangedEvent;

class ProjectInspectorWindow
{
public:
    void OnImGuiRender();
    void OnEvent(Event& _event);

private:
    void OnProjectChangedEvent_(const ProjectChangedEvent& _event);
    void OnFocusingShaderItemChagnedEvent_(const FocusingShaderItemChagnedEvent& _event);

    void Widget_ProjectName_();

    // project operator
    void NewShaderItem_() const;
    void CloneShaderItem_(StringView _name) const;
    void RemoveShaderItem_(StringView _name) const;

    // project
    Project* m_pProjectCache   = nullptr;
    String   m_projectNameEdit = {};

    // shader item
    const ShaderItem* m_pFocusingShaderItemCache = nullptr;
};
