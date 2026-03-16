#pragma once

struct FileItem;

class ProjectChangedEvent : public Event
{
    JAM_EVENT_BODY(ProjectChangedEvent);

public:
    explicit ProjectChangedEvent();
    [[nodiscard]] String ToString() const override;
};

class FocusingShaderItemChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingShaderItemChangedEvent);

public:
    explicit FocusingShaderItemChangedEvent();
    [[nodiscard]] String ToString() const override;
};

class ProjectRenamedEvent : public Event
{
    JAM_EVENT_BODY(ProjectRenamedEvent);

public:
    explicit ProjectRenamedEvent();
    [[nodiscard]] String ToString() const override;
};

class ProjectDirtyChangedEvent : public Event
{
    JAM_EVENT_BODY(ProjectDirtyChangedEvent);

public:
    explicit ProjectDirtyChangedEvent();
    [[nodiscard]] String ToString() const override;
};
