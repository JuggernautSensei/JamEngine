#include "pch.h"

#include "VertexLayoutEntry.h"

namespace Jam
{

VertexLayoutEntry::VertexLayoutEntry(
    const StringView    _name,
    const VertexLayout& _vl)
    : m_name(_name.ToString())
      , m_vl(_vl)
{
}

StringView VertexLayoutEntry::GetName() const
{
    return m_name;
}

const VertexLayout& VertexLayoutEntry::GetVertexLayout() const
{
    return m_vl;
}

void VertexLayoutMap::Emplace(
    const StringView    _name,
    const VertexLayout& _vl)
{
    const Ref<VertexLayoutEntry> vle = MakeRef<VertexLayoutEntry>(_name, _vl);
    const auto [_, bOk]              = m_container.emplace(_name.ToString(), vle);
    JAM_ASSERT(bOk, "VertexLayoutEntry with name '{}' already exists", _name);
}

void VertexLayoutMap::Insert(
    const Ref<VertexLayoutEntry>& _vle)
{
    const auto [_, bOk] = m_container.emplace(_vle->GetName().ToString(), _vle);
    JAM_ASSERT(bOk, "VertexLayoutEntry with name '{}' already exists", _vle->GetName());
}

void VertexLayoutMap::InsertOrReplace(
    const Ref<VertexLayoutEntry>& _vle)
{
    const auto it = m_container.find(_vle->GetName());
    if (it != m_container.end())
    {
        it->second = _vle;
    }
    else
    {
        m_container.emplace(_vle->GetName().ToString(), _vle);
    }
}

void VertexLayoutMap::Remove(
    StringView _name)
{
    const size_t numErased = m_container.erase(_name);
    JAM_ASSERT(numErased > 0, "VertexLayoutEntry with name '{}' does not exist", _name);
}

void VertexLayoutMap::Clear()
{
    m_container.clear();
}

Ref<VertexLayoutEntry> VertexLayoutMap::Get(
    const StringView _name) const
{
    const auto it = m_container.find(_name);
    JAM_ASSERT(it != m_container.end(), "VertexLayoutEntry with name '{}' does not exist", _name);
    return it->second;
}

bool VertexLayoutMap::Has(
    const StringView _name) const
{
    return m_container.contains(_name);
}

void VertexLayoutMap::WriteToArchive(
    ArchiveWriter& _archive) const
{
    for (const auto& [name, vle]: m_container)
    {
        _archive.OpenAtBack();
        _archive.WriteField("name", name);
        _archive.WriteField("vertexLayout", vle->GetVertexLayout());
        _archive.Close();
    }
}

Error VertexLayoutMap::ReadFromArchive(
    ArchiveReader& _archive)
{
    return _archive.ForEach(
        [this](const SizeT, ArchiveReader& _archive) -> Error
        {
            StringView name = {};
            if (const Error err = _archive.ReadField("name", name))
            {
                return err;
            }

            VertexLayout vl = {};
            if (const Error err = _archive.ReadField("vertexLayout", vl))
            {
                return err;
            }

            Emplace(name, vl);
            return Error::None;
        });
}

const StringHashMap<Ref<VertexLayoutEntry>>& VertexLayoutMap::GetContainer() const
{
    return m_container;
}

}   // namespace Jam