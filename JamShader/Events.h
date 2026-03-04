#pragma once

class ProjectChangedEvent : public Event
{
    JAM_EVENT_BODY(ProjectChangedEvent);

public:
    explicit ProjectChangedEvent(const Ref<Project>& _pProject);
    [[nodiscard]] const Ref<Project>& GetProjectOrNull() const;
    [[nodiscard]] String              ToString() const override;

private:
    Ref<Project> m_pProject = nullptr;
};

class ProjectRenamedEvent : public Event
{
    JAM_EVENT_BODY(ProjectRenamedEvent);

public:
    ProjectRenamedEvent(StringView _oldPath, StringView _newPath);
    [[nodiscard]] StringView GetOldPath() const;
    [[nodiscard]] StringView GetNewPath() const;
    [[nodiscard]] String     ToString() const override;

private:
    String m_oldPath = {};
    String m_newPath = {};
};

class FocusingShaderItemChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingShaderItemChangedEvent);

public:
    explicit FocusingShaderItemChangedEvent(const Ref<ShaderItem>& _pShaderItem);
    [[nodiscard]] const Ref<ShaderItem>& GetShaderItemOrNull() const;
    [[nodiscard]] String                 ToString() const override;

private:
    Ref<ShaderItem> m_pShaderItem = nullptr;
};