#pragma once

namespace jam
{

class VertexLayoutEntry : public RefCounted
{
public:
    explicit VertexLayoutEntry(StringView _name, const VertexLayout& _vl);

    [[nodiscard]] StringView          GetName() const;
    [[nodiscard]] const VertexLayout& GetVertexLayout() const;

private:
    std::string  m_name;
    VertexLayout m_vl;
};

}   // namespace jam