#pragma once
#include "Entity.h"
#include "IEditorCommand.h"
#include "JsonReader.h"
#include "JsonWriter.h"

namespace jam
{

class SetFocusedEntityCommand : public IEditorCommand
{
public:
    explicit SetFocusedEntityCommand(Entity _newFocusedEntity);
    void Execute() override;
    void Undo() override;

private:
    Entity m_focusedEntity     = Entity::s_null;
    Entity m_prevFocusedEntity = Entity::s_null;
};

class CreateEntityCommand : public IEditorCommand
{
public:
    void Execute() override;
    void Undo() override;

private:
    Entity m_createdEntity = Entity::s_null;
};

class CloneEntityCommand : public IEditorCommand
{
public:
    explicit CloneEntityCommand(Entity _entityToClone);
    void Execute() override;
    void Undo() override;

private:
    Entity m_entityToClone = Entity::s_null;
    Entity m_clonedEntity  = Entity::s_null;
};

class DestroyEntityCommand : public IEditorCommand
{
public:
    explicit DestroyEntityCommand(Entity _entity);
    void Execute() override;
    void Undo() override;

private:
    [[nodiscard]] Json SerializeEntity_(Entity _entity) const;
    void               SerializeEntity_Recursive_(Entity _entity, JsonWriter _writer) const;
    void               DeserializeEntity_(const Json& _json) const;

    Entity m_entity       = Entity::s_null;
    Json   m_entitiesJson = {};
};

template<typename T>
class AddComponentCommand : public IEditorCommand
{
public:
    explicit AddComponentCommand(const Entity _entity, T&&... _args)
        : m_entity(_entity)
        , m_component(std::forward<T>(_args)...)
    {
    }

    void Execute() override
    {
        m_entity.CreateComponent<T>(std::move(m_component));
    }

    void Undo() override
    {
        m_entity.RemoveComponent<T>();
    }

private:
    Entity m_entity;
    T      m_component;
};

template<typename T>
class EditComponentCommand : public IEditorCommand
{
public:
    explicit EditComponentCommand(
        const Entity _entity,
        const T&     _newValue)
        : m_entity(_entity)
        , m_value(_newValue)
    {
        m_prevValue = m_entity.GetComponent<T>();
    }

    void Execute() override
    {
        T& component = m_entity.GetComponent<T>();
        component    = m_value;
    }

    void Undo() override
    {
        T& component = m_entity.GetComponent<T>();
        component    = m_prevValue;
    }

private:
    Entity m_entity;
    T      m_value;
    T      m_prevValue;
};

template<typename T>
class RemoveComponentCommand : public IEditorCommand
{
public:
    explicit RemoveComponentCommand(
        const Entity _entity)
        : m_entity(_entity)
    {
        m_component = m_entity.GetComponent<T>();
    }
    void Execute() override
    {
        m_entity.RemoveComponent<T>();
    }
    void Undo() override
    {
        m_entity.CreateComponent<T>(std::move(m_component));
    }

private:
    Entity m_entity;
    T      m_component;
};

}   // namespace jam