#pragma once
#include "Model.h"

struct aiNode;
struct aiScene;
struct aiMesh;

namespace jam
{

// assimp 가 지원하는 모델 포맷 임포터
class ModelImporter
{
public:
    bool            Import(const fs::path& _path);
    NODISCARD const std::vector<RawModelElement>& GetRawModelParts() const;

private:
    void Clear_();
    void ProcessNode_(const aiNode* node, const aiScene* scene);
    void ProcessMesh_(const aiMesh* mesh, const aiScene* scene);

    std::vector<RawModelElement> m_rawModelElems;   // 노드 데이터
    bool                         m_bImported = false;
};

}   // namespace jam