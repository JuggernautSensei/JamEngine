#pragma once
#include "Material.h"
#include "Mesh.h"

namespace jam
{

struct RawModelElement
{
    std::string  name;
    MeshGeometry rawMesh;
    Material     material;
};

struct ModelElement
{
    std::string name;
    Mesh        mesh;
    Material    material;
};

class Model
{
public:
    static NODISCARD Model Create(std::span<const ModelElement> _parts);
    static NODISCARD Model Create(std::span<const RawModelElement> _parts, eVertexType _vertexType, eTopology _topology);
    static NODISCARD std::optional<Model> CreateFromFile(const fs::path& _filePath, eVertexType _vertexType, eTopology _topology);

    NODISCARD std::vector<ModelElement>::iterator begin() { return m_elements.begin(); }
    NODISCARD std::vector<ModelElement>::iterator end() { return m_elements.end(); }
    NODISCARD std::vector<ModelElement>::const_iterator begin() const { return m_elements.cbegin(); }
    NODISCARD std::vector<ModelElement>::const_iterator end() const { return m_elements.cend(); }

private:
    std::vector<ModelElement> m_elements;
};

}   // namespace jam