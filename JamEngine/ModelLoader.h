#pragma once
#include "Model.h"

namespace jam
{

struct ModelLoadData
{
    std::vector<RawModelNode> nodes;
    eTopology                 topology   = eTopology::TriangleList;
    eVertexType               vertexType = eVertexType::Vertex3;
};

class ModelLoader
{
public:
    bool Load(const fs::path& _path);
    NODISCARD const ModelLoadData& GetLoadData() const;

private:
    void Clear_();

    ModelLoadData m_loadData;
    bool          m_bLoaded = false;
};

}   // namespace jam