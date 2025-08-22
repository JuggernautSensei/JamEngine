#pragma once
#include "ModelImporter.h"

namespace jam
{

class ModelExporter
{
public:
    void           Load(std::span<const ModelNodeData> _nodes);
    bool           Load(const Model& _model);
    bool           Export(const fs::path& _path) const;
    NODISCARD bool IsLoaded() const { return m_nodes.empty() == false; }

private:
    void Clear_();

    std::vector<ModelNodeData> m_nodes;
};

}   // namespace jam
