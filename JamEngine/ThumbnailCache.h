#pragma once
#include "LRUCache.h"
#include "Events.h"

namespace jam
{

class Event;

class ThumbnailCache
{
public:
    ThumbnailCache();
    [[nodiscard]] Ref<ImageTexture> GetOrPlaceholder(const fs::path& _path);   // return placeholder if not found.
    void                                 OnEvent(Event& _event);

private:
    void                                      GatherThumbnailsFrom_(const fs::path& _dir);
    void                                      InsertThumbnail_(const fs::path& _path);
    [[nodiscard]] Optional<Ref<ImageTexture>> CreateThumbnail_(const fs::path& _path) const;
    [[nodiscard]] Ref<ImageTexture>           CreateImageTexture_(Int32 _w, Int32 _h, CMemView _initData) const;
    void                                      OnFileModifiedEvent_(const FileModifiedEvent& _event);

    LRUCache<fs::path, Ref<ImageTexture>> m_cache;
    Ref<ImageTexture>                          m_pPlaceholderThumbnail;
    Vector<fs::directory_entry>                m_tmpEntries;   // temp storage to avoid reallocs during gathering
};

}   // namespace jam
