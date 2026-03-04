#pragma once
#include "Entity.h"
#include "Event.h"

namespace Jam
{
struct DirEntry;
class VertexLayoutEntry;
struct FileEntry;

class FocusingEntityChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingEntityChangedEvent);

public:
    explicit FocusingEntityChangedEvent(Entity _entity);
    [[nodiscard]] Entity GetEntity() const;
    [[nodiscard]] String ToString() const override;

private:
    Entity m_entity = Entity::k_null;
};

class FocusingFileEntryChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingFileEntryChangedEvent);

public:
    explicit FocusingFileEntryChangedEvent(const Ref<FileEntry>& _pFileEntry);
    [[nodiscard]] const Ref<FileEntry>& GetFileEntry() const;
    [[nodiscard]] String                ToString() const override;

private:
    Ref<FileEntry> m_pEntry = nullptr;
};

class FocusingDirEntryChangedEvent : public Event
{
    JAM_EVENT_BODY(FocusingDirEntryChangedEvent);

public:
    explicit FocusingDirEntryChangedEvent(const Ref<DirEntry>& _pEntry);
    [[nodiscard]] const Ref<DirEntry>& GetDirEntry() const;
    [[nodiscard]] String               ToString() const override;

private:
    Ref<DirEntry> m_pEntry = nullptr;
};

}   // namespace Jam