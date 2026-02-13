#include "pch.h"
#include "SceneSerializer.h"
#include "ReflectionDB.h"
#include "Scene.h"

namespace Jam
{
Error DeserializeComponent_(
    const Ref<Scene>&                  _pScene,
    const ComponentMetadata&           _meta,
    ArchiveReader&                     _reader,
    const Flags<eSceneDeserializeMode> _flags)
{
    UID uid = {};
    if (const Error err = _reader.ReadField("uid", uid))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot read entity UID for component '{}'. {}", _meta.typeName, err.ToErrorMessage());
        return err;
    }

    Optional<Entity> optEntity = _pScene->FindEntity(uid);
    if (!optEntity)
    {
        JAM_LOG.Error("Scene deserialization failed: entity with UID {} not found for component '{}'.", uid, _meta.typeName);
        return Error::RuntimeError;
    }
    const Entity entity = optEntity.GetValue();

    if (const Error err = _reader.EnterField("data"))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot enter component data field for component '{}' of entity UID {}. {}", _meta.typeName, uid.GetValue(), err.ToErrorMessage());
        if (_flags & eSceneDeserializeMode::RemoveComponentOnDeserializeFailure)
        {
            _reader.ClearLastError();   // handled
            return Error::None;
        }

        return err;
    }

    if (!_meta.Has(entity))
    {
        _meta.Create(entity);
    }

    JAM_ASSERT(_reader.IsFine(), "ArchiveReader is not in a fine state before deserializing component.");

    _reader.ClearLastError();   // for collect component deserialization error
    Error err   = Error::None;
    void* pData = _meta.Get(entity);
    if (_meta.Deserialize)
    {
        _meta.Deserialize(pData, _reader, _pScene, entity);
        err = _reader.GetLastError();

        if (err)   // on error.
        {
            JAM_LOG.Error("Scene deserialization failed: cannot deserialize component '{}' for entity UID {}. {}", _meta.typeName, uid.GetValue(), err.ToErrorMessage());
            if (_flags & eSceneDeserializeMode::RemoveComponentOnDeserializeFailure)
            {
                _meta.Remove(entity);
                _reader.ClearLastError();   // handled
            }
        }
    }

    _reader.Exit();
    return err;
}

Error DeserilaizeComponents_(
    const Ref<Scene>&            _pScene,
    ArchiveReader&               _reader,
    Flags<eSceneDeserializeMode> _flags)
{
    StringView typeName = {};
    if (const Error err = _reader.ReadField("typeName", typeName))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot read component type typeName. {}", err.ToErrorMessage());
        return err;
    }

    if (!JAM_REFLECTION_DB.IsComponentReflected(typeName))
    {
        JAM_LOG.Error("Scene deserialization failed: component type '{}' is not registered in ReflectionDB", typeName);
        if (_flags & eSceneDeserializeMode::RemoveComponentOnDeserializeFailure)
        {
            return Error::None;
        }
        return Error::RuntimeError;
    }

    const ComponentMetadata& meta = JAM_REFLECTION_DB.GetComponentMetadata(typeName);
    if (const Error err = _reader.EnterField("values"))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot enter component values field. {}", err.ToErrorMessage());
        return err;
    }

    const Error err = _reader.ForEach(
        [&_pScene, &meta, _flags](const SizeT, ArchiveReader& _reader) -> Error
        {
            return DeserializeComponent_(_pScene, meta, _reader, _flags);
        });

    _reader.Exit();
    return err;
}

void SerializeScene(
    ArchiveWriter&    _writer,
    const Ref<Scene>& _pScene)
{
    _writer.WriteField("sceneType", _pScene->GetTypeName());

    // entities. gather UIDs and handle-Entity map
    HashMap<entt::entity, Entity> handleEntityMap = {};
    const HashMap<UID, Entity>    entities        = _pScene->GetEntities();

    Vector<UID> uids = {};
    uids.reserve(NumericCast<SizeT>(entities.size()));
    for (const auto& [uid, entity]: entities)
    {
        uids.emplace_back(uid);
        handleEntityMap[entity.GetHandle()] = entity;
    }
    _writer.WriteField("entities", std::move(uids));

    // components
    _writer.OpenField("components");   // components array
    const SceneRegistry& reg = _pScene->GetRegistry();
    for (const auto [enttTypeId, storage]: reg.storage())
    {
        const ComponentMetadata& meta = JAM_REFLECTION_DB.GetComponentMetadataByEnttHash(enttTypeId);

        _writer.OpenAtBack();                            // component entry
        _writer.WriteField("typeName", meta.typeName);   // type typeName
        _writer.OpenField("values");                     // open values array
        for (const EntityHandle handle: storage)
        {
            const Entity entity    = handleEntityMap.at(handle);
            const void*  pCmpValue = storage.value(handle);
            _writer.OpenAtBack();                         // component value
            _writer.WriteField("uid", entity.GetUID());   // entity uid
            _writer.OpenField("data");                    // component data

            if (meta.Serialize)
            {
                meta.Serialize(pCmpValue, _writer, _pScene, entity);
            }
            else
            {
                _writer.Write(nullptr);
            }

            _writer.Close();
            _writer.Close();
        }
        _writer.Close();
        _writer.Close();
    }
    _writer.Close();
}

Expect<Ref<Scene>> DeserializeScene(
    ArchiveReader&               _reader,
    Flags<eSceneDeserializeMode> _flags)
{
    StringView sceneTypeName = {};
    if (const Error err = _reader.ReadField("sceneType", sceneTypeName))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot read scene type typeName. {}", err.ToErrorMessage());
        return err;
    }

    if (!JAM_REFLECTION_DB.IsSceneReflected(sceneTypeName))
    {
        JAM_LOG.Error("Scene deserialization failed: scene type '{}' is not registered in ReflectionDB", sceneTypeName);
        return Error::RuntimeError;
    }

    Ref<Scene> pScene = JAM_REFLECTION_DB.GetSceneMetadata(sceneTypeName).Create();

    Vector<UID> entityUids = {};
    if (const Error err = _reader.ReadField("entities", entityUids))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot read entity UIDs. {}", err.ToErrorMessage());
        return err;
    }

    const Entity root    = pScene->GetRootEntity();
    const UID    rootUid = root.GetUID();
    for (const UID uid: entityUids)
    {
        if (uid != rootUid)
        {
            pScene->CreateEntity(uid);
        }
    }

    if (const Error err = _reader.EnterField("components"))
    {
        JAM_LOG.Error("Scene deserialization failed: cannot enter components field. {}", err.ToErrorMessage());
        return err;
    }

    const Error err = _reader.ForEach(
        [&pScene, _flags](const SizeT, ArchiveReader& _reader) -> Error
        {
            return DeserilaizeComponents_(pScene, _reader, _flags);
        });

    _reader.Exit();

    if (err)
    {
        return err;
    }

    return pScene;
}
}   // namespace Jam