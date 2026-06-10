#pragma once
#include "StringKeyedContainers.h"
#include "VertexLayout.h"

namespace Jam
{
//
//class VertexLayoutEntry : public RefCounted
//{
//public:
//    VertexLayoutEntry() = default;
//
//    VertexLayoutEntry(
//        const String&       _name,
//        const VertexLayout& _vl)
//        : m_name(_name)
//        , m_vl(_vl)
//    {
//    }
//
//    void SetName(
//        const String& _name)
//    {
//        m_name = _name;
//    }
//
//    void SetVertexLayout(
//        const VertexLayout& _vl)
//    {
//        m_vl = _vl;
//    }
//
//    [[nodiscard]] StringView GetName() const
//    {
//        return m_name;
//    }
//
//    [[nodiscard]] const VertexLayout& GetVertexLayout() const
//    {
//        return m_vl;
//    }
//
//private:
//    String       m_name = {};
//    VertexLayout m_vl   = {};
//};
//
//class VertexLayoutEntryManager
//{
//public:
//    void Add(const Ref<VertexLayoutEntry>& _pEntry)
//    {
//        JAM_ASSERT(_pEntry, "Cannot add null VertexLayoutEntry.");
//        const StringView name = _pEntry->GetName();
//        JAM_ASSERT(!Contains(name), "VertexLayoutEntry with name '{}' already exists.", name);
//
//        const auto ret = m_entries.PushBack(name, _pEntry);
//        JAM_ASSERT(ret.bSucceeded, "Failed to add VertexLayoutEntry with name '{}'.", name);
//    }
//
//    Ref<VertexLayoutEntry> Emplace(const StringView _name, const VertexLayout& _vl)
//    {
//        JAM_ASSERT(!Contains(_name), "VertexLayoutEntry with name '{}' already exists.", _name);
//
//        Ref<VertexLayoutEntry> pEntry = MakeRef<VertexLayoutEntry>(_name, _vl);
//        const auto             ret    = m_entries.PushBack(_name, pEntry);
//        JAM_ASSERT(ret.bSucceeded, "Failed to add VertexLayoutEntry with name '{}'.", _name);
//        return pEntry;
//    }
//
//    void Rename(const StringView _oldName, const StringView _newName)
//    {
//        if (_oldName == _newName)
//        {
//            return;
//        }
//
//        JAM_ASSERT(!Contains(_newName), "VertexLayoutEntry with name '{}' already exists.", _newName);
//
//        const auto it = m_entries.Find(_oldName);
//        JAM_ASSERT(it != m_entries.End(), "VertexLayoutEntry with name '{}' does not exist.", _oldName);
//
//        Ref<VertexLayoutEntry>& pEntry = it->value;
//        pEntry->SetName(_newName);
//
//        const auto [_, bRet] = m_entries.Insert(it, _newName, pEntry);
//        JAM_ASSERT(bRet, "Failed to rename VertexLayoutEntry from '{}' to '{}'.", _oldName, _newName);
//        m_entries.Remove(it);
//    }
//
//    void Rename(const Ref<VertexLayoutEntry>& _pEntry, const StringView _newName)
//    {
//        JAM_ASSERT(_pEntry, "Cannot rename null VertexLayoutEntry.");
//        Rename(_pEntry->GetName(), _newName);
//    }
//
//    void Remove(const StringView _name)
//    {
//        const auto removeResult = m_entries.Remove(_name);
//        JAM_ASSERT(removeResult.bSucceeded, "VertexLayoutEntry with name '{}' does not exist.", _name);
//    }
//
//    void Remove(const Ref<VertexLayoutEntry>& _pEntry)
//    {
//        JAM_ASSERT(_pEntry, "Cannot remove null VertexLayoutEntry.");
//        Remove(_pEntry->GetName());
//    }
//
//    [[nodiscard]] bool Contains(const StringView _name) const
//    {
//        return m_entries.Contains(_name);
//    }
//
//    [[nodiscard]] const Ref<VertexLayoutEntry>& Get(const StringView _name) const
//    {
//        const auto it = m_entries.Find(_name);
//        JAM_ASSERT(it != m_entries.End(), "VertexLayoutEntry with name '{}' does not exist.", _name);
//        return it->value;
//    }
//
//    void Splice(const StringView _from, const StringView _to)
//    {
//        JAM_ASSERT(Contains(_from), "VertexLayoutEntry with name '{}' does not exist.", _from);
//        JAM_ASSERT(Contains(_to), "VertexLayoutEntry with name '{}' does not exist.", _to);
//        const auto itFrom = m_entries.Find(_from);
//        const auto itTo   = m_entries.Find(_to);
//        m_entries.Move(itTo, itFrom);
//    }
//
//    void Splice(const Ref<VertexLayoutEntry>& _pFrom, const Ref<VertexLayoutEntry>& _pTo)
//    {
//        JAM_ASSERT(_pFrom, "Cannot splice from null VertexLayoutEntry.");
//        JAM_ASSERT(_pTo, "Cannot splice to null VertexLayoutEntry.");
//        Splice(_pFrom->GetName(), _pTo->GetName());
//    }
//
//    [[nodiscard]] const StringHashMap<Ref<VertexLayoutEntry>>& GetEntries() const
//    {
//        return m_entries;
//    }
//
//    void WriteToArchive(ArchiveWriter& _archive) const
//    {
//        _archive.BeginArray();
//        for (const auto& [name, pEntry]: m_entries)
//        {
//            _archive.AddElement();
//            _archive.BeginObject();
//            _archive.WriteField("name", name);
//            _archive.WriteField("vertexLayout", pEntry->GetVertexLayout());
//            _archive.EndObject();
//        }
//        _archive.EndArray();
//    }
//
//    Error ReadFromArchive(ArchiveReader& _archive)
//    {
//        m_entries.Clear();
//
//        return _archive.ForEachArray(Bind(
//            [this](SizeT, ArchiveReader& _ar) -> Error {
//                String name = {};
//                if (const Error err = _ar.ReadField("name", name))
//                {
//                    return err;
//                }
//
//                VertexLayout vertexLayout = {};
//                if (const Error err = _ar.ReadField("vertexLayout", vertexLayout))
//                {
//                    return err;
//                }
//
//                if (Contains(name))
//                {
//                    return Error::InvalidData;
//                }
//
//                Emplace(name, vertexLayout);
//                return Error::None;
//            }));
//    }
//
//private:
//    StringHashMap<Ref<VertexLayoutEntry>> m_entries = {};
//};

//JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(VertexLayoutEntryManager);

}   // namespace Jam