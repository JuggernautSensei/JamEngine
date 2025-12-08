#pragma once
#include "Entity.h"

namespace jam
{

class DestroyNotLeafEntityDialog
{
public:
    explicit DestroyNotLeafEntityDialog(Entity _entity);
    void operator()() const;

private:
    Entity m_entity = Entity::s_null;
};

}   // namespace jam