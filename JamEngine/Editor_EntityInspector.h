#pragma once
#include "ComponentDB.h"

namespace jam
{

class Editor_EntityInspector
{
public:
    void OnImGuiRender(bool& _pShow) const;

private:
    void DrawRemoveComponentPopup_() const;
    void DrawAddComponentPopup_() const;

    void AddComponent_(UInt32 _typeID, Entity _entity) const;
    void RemoveComponent_(UInt32 _typeID, Entity _entity) const;
};

}   // namespace jam
