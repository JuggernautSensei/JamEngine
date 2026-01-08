#include "pch.h"

#include "VertexLayoutEntry.h"

namespace jam
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

}   // namespace jam