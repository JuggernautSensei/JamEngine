#pragma once

struct FileItem;

class ProjectChangedEvent : public Event
{
    JAM_EVENT_BODY(ProjectChangedEvent);

public:
    explicit ProjectChangedEvent(Project* _pProject);
    [[nodiscard]] Project* GetProjectOrNull() const;
    [[nodiscard]] String   ToString() const override;

private:
    Project* m_pProject = nullptr;
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

class FocusingFileItemChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingFileItemChangedEvent);

public:
    explicit FocusingFileItemChangedEvent(FileItem* _pFocusingFileItem);
    [[nodiscard]] FileItem* GetFocusingFileItemOrNull() const;
    [[nodiscard]] String    ToString() const override;

private:
    FileItem* m_pFocusingFileItem = nullptr;
};

class FocusingShaderItemChagnedEvent : public Event
{
    JAM_EVENT_BODY(FocusingShaderItemChagnedEvent);

public:
    explicit FocusingShaderItemChagnedEvent(StringView _shaderItemName);
    [[nodiscard]] StringView GetFocusingShaderItemNameOrEmpty() const;
    [[nodiscard]] String     ToString() const override;

private:
    String m_shaderItemName = {};
};