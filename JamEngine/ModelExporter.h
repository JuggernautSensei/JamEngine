#pragma once
#include "ModelImporter.h"
#include "Vertex.h"

namespace jam
{

class ModelExporter
{
public:
    void Load(std::span<const RawModelElement> _parts);
    bool Load(const Model& _model, eVertexType _type);
    bool Export(const fs::path& _path) const;

private:
    void Clear_();

    std::vector<RawModelElement> m_elements;
    bool                         m_bExported = false;
};

}   // namespace jam
