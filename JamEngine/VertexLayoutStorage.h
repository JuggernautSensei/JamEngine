#pragma once
#include "VertexLayout.h"

namespace jam
{

class VertexLayoutEntry : public RefCounted
{
public:
    VertexLayoutEntry()
    {
    }

    VertexLayoutEntry(const std::string_view _name, const VertexLayout& _layout)
        : m_name(_name)
        , m_vertexLayout(_layout)
    {
    }

    [[nodiscard]] const std::string& GetName() const
    {
        return m_name;
    }

    [[nodiscard]] const VertexLayout& GetVertexLayout() const
    {
        return m_vertexLayout;
    }

private:
    std::string  m_name;
    VertexLayout m_vertexLayout;
};

class VertexLayoutStorage
{
public:
    void                                 Add(std::string_view _name, const VertexLayout& _vl);
    void                                 Remove(std::string_view _name);
    [[nodiscard]] bool                   Has(std::string_view _name) const;
    [[nodiscard]] Ref<VertexLayoutEntry> Get(std::string_view _name) const;
    void                                 Clear();

    [[nodiscard]] decltype(auto) begin() const { return m_container.begin(); }
    [[nodiscard]] decltype(auto) end() const { return m_container.end(); }
    [[nodiscard]] decltype(auto) begin() { return m_container.begin(); }
    [[nodiscard]] decltype(auto) end() { return m_container.end(); }

private:
    HashMap<std::string, Ref<VertexLayoutEntry>> m_container;
};

}   // namespace jam
