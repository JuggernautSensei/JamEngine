#pragma once

namespace Jam
{

class VertexLayoutEntry : public RefCounted
{
public:
    explicit VertexLayoutEntry(StringView _name, const VertexLayout& _vl);

    [[nodiscard]] StringView          GetName() const;
    [[nodiscard]] const VertexLayout& GetVertexLayout() const;

private:
    String       m_name = {};
    VertexLayout m_vl   = {};
};

class VertexLayoutMap
{
public:
    void Emplace(StringView _name, const VertexLayout& _vl);
    void Insert(const Ref<VertexLayoutEntry>& _vle);
    void InsertOrReplace(const Ref<VertexLayoutEntry>& _vle);
    void Remove(StringView _name);
    void Clear();

    [[nodiscard]] Ref<VertexLayoutEntry> Get(StringView _name) const;
    [[nodiscard]] bool                   Has(StringView _name) const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    [[nodiscard]] const StringHashMap<Ref<VertexLayoutEntry>>& GetContainer() const;

private:
    StringHashMap<Ref<VertexLayoutEntry>> m_container = {};
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(VertexLayoutMap);

}   // namespace Jam