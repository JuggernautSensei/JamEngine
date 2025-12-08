#pragma once
#include "PopupDialog.h"
#include "VertexLayoutStorage.h"

namespace jam
{

enum class eMeshType
{
    Cube,
    Sphere,
    Plane,
    Circle,
    Cone,
    Cylinder
};

class CreatePrimitiveDialog
{
public:
    explicit CreatePrimitiveDialog(const fs::path& _parentPath, eMeshType _meshType);
    void operator()();

private:
    void DrawCreateCubeTable_();
    void DrawCreateSphereTable_();
    void DrawCreatePlaneTable_();
    void DrawCreateCircleTable_();
    void DrawCreateConeTable_();
    void DrawCreateCylinderTable_();

    bool OnClickCreateCubeTableOkButton_(float _width, float _height, float _depth);
    bool OnClickCreateSphereTableOkButton_(float _radius, Int32 _subdivisionLevel);
    bool OnClickCreatePlaneTableOkButton_(float _width, float _depth);
    bool OnClickCreateCircleTableOkButton_(float _radius, Int32 _numTri);
    bool OnClickCreateConeTableOkButton_(float _height, float _radius, Int32 _numBottomTri);
    bool OnClickCreateCylinderTableOkButton_(float _height, float _radius, Int32 _numBottomTri);

    fs::path               m_parentPath;
    eMeshType              m_meshType = eMeshType::Cube;
    std::string            m_buf;
    Ref<VertexLayoutEntry> m_pEntry;

    PopupDialog m_popup;

    float m_float0 = 1.f;
    float m_float1 = 1.f;
    float m_float2 = 1.f;
    Int32 m_int0   = 1;

    constexpr static ImVec2 k_tableSize = ImVec2(500.f, 0.f);
};

}   // namespace jam