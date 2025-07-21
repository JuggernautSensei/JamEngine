#pragma once
#include "Model.h"

namespace jam
{

class ModelLoader
{
public:
    bool            Load(const fs::path& _path);
    NODISCARD const std::vector<RawModelElement>& GetRawModelParts() const;

private:
    void Clear_();

    std::vector<RawModelElement> m_rawModelElems;   // 노드 데이터
    bool                         m_bLoaded = false;
};

}   // namespace jam