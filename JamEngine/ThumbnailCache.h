#pragma once
#include "LRUCache.h"
#include "Events.h"

namespace Jam
{
class Texture;

class Event;

class ThumbnailCache
{
public:
    ThumbnailCache();
    [[nodiscard]] Ref<Texture> GetOrPlaceholder(StringView _path);   // return placeholder if not found.
    void                       OnEvent(Event& _event);

private:
    void                                 GatherThumbnailsFrom_(StringView _dir);
    void                                 InsertThumbnail_(StringView _path);
    [[nodiscard]] Optional<Ref<Texture>> CreateThumbnail_(StringView _path) const;
    [[nodiscard]] Ref<Texture>           CreateTexture_(Int32 _w, Int32 _h, CMemoryBlock _initData) const;
    void                                 OnFileModifiedEvent_(const FileModifiedEvent& _event);

    LRUCache<String, Ref<Texture>> m_cache;
    Ref<Texture>                   m_pPlaceholderThumbnail;
    Vector<String>                 m_pathes;   // temp storage to avoid reallocs during gathering
};

}   // namespace Jam
