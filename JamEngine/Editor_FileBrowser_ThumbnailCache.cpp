#include "pch.h"

#include "Editor_FileBrowser.h"

namespace jam
{

void Editor_FileBrowser::ThumbnailCache::Init()
{
    m_thumbnails.clear();
    m_pathToItem.clear();
    m_end = 0;
    m_cap = 0;

    m_pDummyThumbnail = MakeRef<Thumbnail>();
    m_pDummyThumbnail->LoadDummy();
}

Ref<Editor_FileBrowser::Thumbnail> Editor_FileBrowser::ThumbnailCache::LoadThumbnail(const fs::path& _path)
{
    // check if already cached
    if (m_pathToItem.contains(_path))
    {
        const Item&                 item = m_pathToItem.at(_path);
        const std::optional<UInt64> opt  = GetLastModifiedTime_(_path);

        if (opt)
        {
            const UInt64 timePoint = opt.value();

            // Check if file was modified
            if (item.timePoint == timePoint)
            {
                // return cached thumbnail
                return m_thumbnails[item.bufferIdx];
            }
            else
            {
                // file modified
                Ref<Thumbnail> pNewThumbnail = MakeRef<Thumbnail>();
                if (pNewThumbnail->Load(_path))
                {
                    m_thumbnails[item.bufferIdx]  = pNewThumbnail;
                    m_pathToItem[_path].timePoint = timePoint;
                }
                return m_thumbnails[item.bufferIdx];   // new thumbnail or old cached if failed to load
            }
        }
        else
        {
            // fail to get mod time - return cached thumbnail
            return m_thumbnails[item.bufferIdx];
        }
    }

    // collect thumbnails in the directory
    CollectThumbnailsInDirectory_(_path.parent_path());

    // return the requested thumbnail if loaded successfully
    if (m_pathToItem.contains(_path))
    {
        const Item& item = m_pathToItem.at(_path);
        return m_thumbnails[item.bufferIdx];
    }
    else
    {
        return m_pDummyThumbnail;
    }
}

void Editor_FileBrowser::ThumbnailCache::CollectThumbnailsInDirectory_(const fs::path& _dirPath)
{
    JAM_ASSERT(fs::is_directory(_dirPath), "Path is not a directory: %s", _dirPath.string().c_str());

    // Count files in directory
    Int32 numFiles = 0;
    for (const fs::directory_entry& _: fs::directory_iterator(_dirPath))
    {
        ++numFiles;
    }

    // Resize buffer if needed
    if (numFiles > m_cap)
    {
        m_thumbnails.clear();
        m_pathToItem.clear();
        m_cap = numFiles + 500;
        m_thumbnails.resize(m_cap);
        m_end = 0;
    }

    for (const fs::directory_entry& entry: fs::directory_iterator(_dirPath))
    {
        const fs::path&             path = entry.path();
        const std::optional<UInt64> opt  = GetLastModifiedTime_(path);

        // check if already cached
        if (m_pathToItem.contains(path) && opt)
        {
            const UInt64 timePoint = opt.value();

            Item& item = m_pathToItem.at(path);
            if (item.timePoint != timePoint)
            {
                // modified - reload thumbnail
                Ref<Thumbnail> pNew = MakeRef<Thumbnail>();
                if (pNew->Load(path))
                {
                    m_thumbnails[item.bufferIdx] = pNew;
                    item.timePoint               = timePoint;
                }
            }

            // not modified or failed to load - continue
            continue;
        }

        const Ref<Thumbnail>& pOld = m_thumbnails[m_end];
        if (pOld && !pOld->IsDummy())
        {
            m_pathToItem.erase(pOld->GetPath());
        }

        // new thumbnail
        Ref<Thumbnail> pNewThumbnail = MakeRef<Thumbnail>();
        if (opt && pNewThumbnail->Load(path))
        {
            const UInt64 timePoint = opt.value();

            // store thumbnail
            m_thumbnails[m_end] = pNewThumbnail;

            // store mapping
            Item item;
            item.bufferIdx     = m_end;
            item.timePoint     = timePoint;
            m_pathToItem[path] = item;
        }
        else
        {
            // failed - use dummy
            m_thumbnails[m_end] = m_pDummyThumbnail;
        }

        m_end = (m_end + 1) % m_cap;
    }
}

std::optional<UInt64> Editor_FileBrowser::ThumbnailCache::GetLastModifiedTime_(const fs::path& _path) const
{
    try
    {
        fs::file_time_type modTime = fs::last_write_time(_path);
        return modTime.time_since_epoch().count();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

}   // namespace jam