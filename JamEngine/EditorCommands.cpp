#include "pch.h"
#include "EditorCommands.h"

#include "ComponentDB.h"
#include "Editor.h"
#include "JsonWriter.h"
#include "SceneManager.h"
#include "SceneNode.h"

namespace jam
{

SetFocusedEntityCommand::SetFocusedEntityCommand(
    const Entity _newFocusedEntity)
    : m_focusedEntity(_newFocusedEntity)
{
}

void SetFocusedEntityCommand::Execute()
{
    m_prevFocusedEntity = Editor::GetFocusedEntity();
    Editor::SetFocusedEntity(m_focusedEntity);
}

void SetFocusedEntityCommand::Undo()
{
    JAM_ASSERT(m_prevFocusedEntity, "Previous focused entity is not valid.");
    Editor::SetFocusedEntity(m_prevFocusedEntity);
}

void CreateEntityCommand::Execute()
{
    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    m_createdEntity    = pScene->CreateEntity();
}

void CreateEntityCommand::Undo()
{
    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    pScene->DestroyEntity(m_createdEntity);
}

CloneEntityCommand::CloneEntityCommand(
    const Entity _entityToClone)
    : m_entityToClone(_entityToClone)
{
}

void CloneEntityCommand::Execute()
{
    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    m_clonedEntity     = pScene->CloneEntity(m_entityToClone);
}

void CloneEntityCommand::Undo()
{
    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    pScene->DestroyEntity(m_clonedEntity);
}

DestroyEntityCommand::DestroyEntityCommand(
    const Entity _entity)
    : m_entity(_entity)
{
}

void DestroyEntityCommand::Execute()
{
    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    m_entitiesJson     = SerializeEntity_(m_entity);
    pScene->DestroyEntity(m_entity);
}

void DestroyEntityCommand::Undo()
{
    DeserializeEntity_(m_entitiesJson);
}

Json DestroyEntityCommand::SerializeEntity_(
    const Entity _entity) const
{
    /*
        array of entities data

        entity data
            [0] = entity UID
            [1] = array of components
                    component = [ name, data ]
    */

    Ref<Scene> pScene = SceneManager::GetActiveScene();
    JAM_ASSERT(_entity, "Entity is not valid.");
    JAM_ASSERT(pScene->IsEntityValid(_entity), "Entity does not belong to the active scene.");

    Json       json;
    JsonWriter writer { json };
    JsonWriter entityWriter = writer.PushBack();   // entity data
    SerializeEntity_Recursive_(_entity, entityWriter);

    return json;
}

void DestroyEntityCommand::SerializeEntity_Recursive_(
    const Entity _entity,
    JsonWriter   _writer) const
{
    // top -> down

    Ref<Scene>& pScene = SceneManager::GetActiveScene();
    JAM_ASSERT(_entity, "Entity is not valid.");

    // [0] = entity UID
    _writer.PushBack(_entity.GetUID());

    // [1] = array of components
    JsonWriter cmpsWriter = _writer.PushBack();

    const ComponentDB&    db   = ComponentDB::GetInstance();
    const entt::registry* pReg = _entity.GetRegistry();
    for (const auto&& [typeId, set]: pReg->storage())
    {
        if (db.Has(typeId, _entity))   // component found
        {
            // component
            JsonWriter cmpWriter = cmpsWriter.PushBack();

            // [0] name
            const StringView name = db.GetComponentName(typeId);
            cmpWriter.PushBack(name);

            // [1] data
            JsonWriter dataWriter = cmpWriter.PushBack();   // data
            if (const void* pValue = db.Get(typeId, _entity))
            {
                db.Serialize(name, pValue, dataWriter, pScene, _entity);
            }
        }
    }

    // serialize children recursively
    const SceneNode& node  = _entity.GetSceneNode();
    Entity           child = node.GetFirstChild();
    while (child)
    {
        JsonWriter childWriter = _writer.PushBack();   // child entity data
        SerializeEntity_Recursive_(child, childWriter);
        child = node.GetNextSibling();
    }
}

void DestroyEntityCommand::DeserializeEntity_(
    const Json& _json) const
{
    // parent 가 먼저 생성됨. (직렬화 자체를 top -> down으로 했기 때문)
    // 그러므로 순차적으로 역직렬화해도 문제가 없음.

    JsonReader reader { _json };
    for (Int32 i = 0; i < reader.GetSize(); ++i)
    {
        Optional<JsonReader> optEntityReader = reader.EnterAt(i);
        JAM_ASSERT(optEntityReader, "Failed to enter entity json.");
        JsonReader entityReader = optEntityReader.value();

        // create entity

        UID uid;
        if (!entityReader.ReadAt(0, uid))
        {
            JAM_ASSERT(0, "Failed to read entity UID from json.");
        }
        Ref<Scene>& pScene = SceneManager::GetActiveScene();
        Entity      entity = pScene->CreateEntity(uid);

        // deserialize components
        Optional<JsonReader> optCmpsReader = entityReader.EnterAt(1);
        JAM_ASSERT(optCmpsReader, "Failed to enter components json.");
        JsonReader cmpsReader = optCmpsReader.value();

        const ComponentDB& db = ComponentDB::GetInstance();
        for (Int32 j = 0; j < cmpsReader.GetSize(); ++j)
        {
            Optional<JsonReader> optCmpReader = cmpsReader.EnterAt(j);
            JAM_ASSERT(optCmpReader, "Failed to enter component json.");
            JsonReader cmpReader = optCmpReader.value();

            // [0] name
            std::string name;
            if (!cmpReader.ReadAt(0, name))
            {
                JAM_ASSERT(0, "Failed to read component name from json.");
            }

            // [1] data
            Optional<JsonReader> optDataReader = cmpReader.EnterAt(1);
            JAM_ASSERT(optDataReader, "Failed to enter component data json.");
            JsonReader dataReader = optDataReader.value();

            // create component if not exists
            if (!db.Has(name, entity))
            {
                db.Create(name, entity);
            }

            // deserialize
            if (void* pValue = db.Get(name, entity))
            {
                db.Deserialize(name, pValue, dataReader, pScene, entity);
            }
        }
    }
}

}   // namespace jam