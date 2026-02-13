#pragma once
#include "LRUCache.h"
#include "Events.h"

namespace Jam
{

class Event;

class ThumbnailCache
{
public:
    ThumbnailCache();
    [[nodiscard]] Ref<Texture> GetOrPlaceholder(const Path& _path);   // return placeholder if not found.
    void                       OnEvent(Event& _event);

private:
    void                                 GatherThumbnailsFrom_(const Path& _dir);
    void                                 InsertThumbnail_(const Path& _path);
    [[nodiscard]] Optional<Ref<Texture>> CreateThumbnail_(const Path& _path) const;
    [[nodiscard]] Ref<Texture>           CreateTexture_(Int32 _w, Int32 _h, CMemoryBlock _initData) const;
    void                                 OnFileModifiedEvent_(const FileModifiedEvent& _event);

    LRUCache<Path, Ref<Texture>> m_cache;
    Ref<Texture>                 m_pPlaceholderThumbnail;
    Vector<fs::directory_entry>  m_tmpEntries;   // temp storage to avoid reallocs during gathering
};

}   // namespace Jam
