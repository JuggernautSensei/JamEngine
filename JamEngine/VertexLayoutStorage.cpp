#include "pch.h"

#include "VertexLayoutStorage.h"

namespace jam
{

void VertexLayoutStorage::Add(const std::string_view _name, const VertexLayout& _vl)
{
    JAM_ASSERT(!Has(_name), "Vertex layout with the given name already exists.");
    const Ref<VertexLayoutEntry> pEntry = MakeRef<VertexLayoutEntry>(_name, _vl);
    m_container[std::string { _name }]    = pEntry;
}

void VertexLayoutStorage::Remove(const std::string_view _name)
{
    JAM_ASSERT(Has(_name), "Vertex layout with the given name does not exist.");
    std::string key { _name };
    m_container.erase(key);
}

bool VertexLayoutStorage::Has(const std::string_view _name) const
{
    std::string key { _name };
    return m_container.contains(key);
}

Ref<VertexLayoutEntry> VertexLayoutStorage::Get(const std::string_view _name) const
{
    JAM_ASSERT(Has(_name), "Vertex layout with the given name does not exist.");
    std::string key { _name };
    return m_container.at(key);
}

void VertexLayoutStorage::Clear()
{
    m_container.clear();
}

}   // namespace jam