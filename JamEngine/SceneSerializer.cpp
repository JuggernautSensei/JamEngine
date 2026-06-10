#include "pch.h"

#include "ComponentDatabase.h"
#include "JsonArchive.h"
#include "Scene.h"
#include "SceneDatabase.h"

namespace Jam
{

namespace
{

    Error DeserializeEntity_(
        Scene&         _scene,
        const UID      _rootUid,
        SizeT          _index,
        ArchiveReader& _reader)
    {
        UID uid = {};
        if (const Error err = _reader.Read(uid))
        {
            JAM_LOG_ERROR("Scene deserialize error: failed to read entity UID at index {}.\n{}", _index, err);
            return err;
        }

        if (uid != _rootUid)
        {
            _scene.CreateEntity(uid);
        }

        return Error::None;
    }

    Error DeserializeComponentEntry_(
        Scene&                              _scene,
        const ComponentDatabase::Data&      _data,
        const Flags<eSceneDeserializeFlags> _flags,
        SizeT                               _index,
        ArchiveReader&                      _reader)
    {
        UID uid = {};
        if (const Error err = _reader.ReadField("uid", uid))
        {
            JAM_LOG_ERROR("Scene deserialize error: failed to read entity UID for component at index {}.\n{}", _index, err);
            return err;
        }

        Entity entity = _scene.FindEntityOrNull(uid);
        if (!entity)
        {
            JAM_LOG_ERROR("Scene deserialize error: entity with UID {} not found for component at index {}.", uid, _index);
            return Error::InvalidData;
        }

        if (!_data.Has(entity))
        {
            _data.Create(entity);
        }

        _reader.EnterField("data");
        if (!_reader.IsNull())
        {
            if (!_data.Deserialize)
            {
                JAM_LOG_ERROR("Scene deserialize error: component type '{}' does not have a Deserialize function, but the data is not null at index {}.", _data.typeName, _index);
                return Error::InvalidData;
            }

            void* pCmp = _data.Get(entity);
            JAM_ASSERT(pCmp, "Scene deserialize assertion failed: component data pointer is null for component type '{}' at index {}.", _data.typeName, _index);

            _reader.ClearLastError();
            _data.Deserialize(pCmp, _reader, &_scene, entity);

            if (const Error err = _reader.GetLastError())
            {
                if (_flags & eSceneDeserializeFlags::SkipComponentDeserializeFail)
                {
                    _data.Remove(entity);
                    JAM_LOG_WARN("Scene deserialize warning: failed to deserialize component type '{}' data at index {}. The component will be removed from the entity to continue deserialization.\n{}", _data.typeName, _index, err);
                    _reader.ClearLastError();
                    _reader.Exit();
                    return Error::None;
                }

                JAM_LOG_ERROR("Scene deserialize error: failed to deserialize component type '{}' data at index {}.\n{}", _data.typeName, _index, err);
                return err;
            }
        }
        _reader.Exit();

        return Error::None;
    }

    Error DeserializeComponent_(
        Scene&                              _scene,
        const Flags<eSceneDeserializeFlags> _flags,
        StringView                          _key,
        ArchiveReader&                      _reader)
    {
        ComponentDatabase& db = GetComponentDatabase();
        if (!db.IsComponentRegistered(_key))
        {
            if (_flags & eSceneDeserializeFlags::SkipComponentDeserializeFail)
            {
                JAM_LOG_WARN("Scene deserialize warning: component type '{}' is not reflected in the component database. The component will be skipped.", _key);
                return Error::None;
            }

            JAM_LOG_ERROR("Scene deserialize error: component type '{}' is not reflected in the component database.", _key);
            return Error::InvalidData;
        }

        const ComponentDatabase::Data& data = db.GetComponentData(_key);
        if (const Error err = _reader.ForEachArray(Bind([&_scene, &data, _flags](const SizeT _index, ArchiveReader& _reader) {
                return DeserializeComponentEntry_(_scene, data, _flags, _index, _reader);
            })))
        {
            JAM_LOG_ERROR("Scene deserialize error: failed to read components array for component type '{}'.\n{}", data.typeName, err);
            return err;
        }

        return Error::None;
    }

    Expect<Scoped<Scene>> LoadFromJson_(
        JsonReader&                         _reader,
        const Flags<eSceneDeserializeFlags> _flags)
    {
        StringView sceneType = {};
        if (const Error err = _reader.ReadField("sceneType", sceneType))
        {
            JAM_LOG_ERROR("Scene deserialize error: failed to read 'sceneType' field. {}", err);
            return err;
        }

        const SceneDatabase& db = GetSceneDatabase();
        if (!db.IsSceneRegistered(sceneType))
        {
            JAM_LOG_ERROR("Scene deserialize error: scene type '{}' is not reflected in the scene database. Please ensure that the scene type is registered with the JAM_SCENE macro in its class definition.", sceneType);
            return Error::ReflectionFailed;
        }

        Scoped<Scene> pScene = db.GetSceneData(sceneType).Make();
        if (const Error err = pScene->Deserialize(_reader, _flags))
        {
            JAM_LOG_ERROR("Scene deserialize error: failed to deserialize scene data. {}", err);
            return err;
        }

        return pScene;
    }

}   // namespace

Expect<Scoped<Scene>> Scene::Load(
    const StringView                    _json,
    const Flags<eSceneDeserializeFlags> _flags)
{
    Expect<JsonReader> reader = JsonReader::Parse(_json);
    if (!reader)
    {
        JAM_LOG_ERROR("Failed to parse JSON data for scene deserialization. {}", reader.GetError());
        return reader.GetError();
    }

    return LoadFromJson_(reader.GetValue(), _flags);
}

Expect<Scoped<Scene>> Scene::LoadFromFile(
    const StringView                    _path,
    const Flags<eSceneDeserializeFlags> _flags)
{
    Expect<JsonReader> reader = JsonReader::ParseFromFile(_path);
    if (!reader)
    {
        JAM_LOG_ERROR("Failed to parse JSON file '{}' for scene deserialization. {}", _path, reader.GetError());
        return reader.GetError();
    }

    return LoadFromJson_(reader.GetValue(), _flags);
}

void Scene::Serialize(ArchiveWriter& _writer)
{
    // resource
    HashMap<entt::entity, Entity> handleToEntity = {};
    handleToEntity.reserve(m_entities.size());

    _writer.BeginObject();

    // scene type
    _writer.WriteField("sceneType", GetTypeName());

    // entities: UID 배열
    _writer.AddField("entities");
    _writer.BeginArray();
    for (const auto [uid, entity]: m_entities)
    {
        _writer.WriteElement(uid);
        handleToEntity.emplace(entity.GetHandle(), entity);
    }
    _writer.EndArray();

    // components: { typeName: [ { uid, data }, ... ], ... }
    _writer.AddField("components");
    _writer.BeginObject();

    ComponentDatabase& db = GetComponentDatabase();
    for (const auto [hash, container]: m_registry.storage())
    {
        const ComponentDatabase::Data& data = db.GetComponentDataByENTTHash(hash);
        if (!data.Serialize || container.empty())
        {
            continue;
        }

        _writer.AddField(data.typeName);
        _writer.BeginArray();
        for (const entt::entity handle: container)
        {
            const auto it = handleToEntity.find(handle);
            JAM_ASSERT(it != handleToEntity.end(),
                       "Scene serialize assertion failed: entity handle not found in handleToEntity map. "
                       "This should never happen because all handles from m_registry.storage() should be present in m_entities.");

            const Entity entity = it->second;
            void*        pCmp   = data.Get(entity);

            _writer.AddElement();
            _writer.BeginObject();
            _writer.WriteField("uid", entity.GetUID());

            _writer.AddField("data");
            if (pCmp)
            {
                data.Serialize(pCmp, _writer, this, entity);
            }
            else
            {
                _writer.Write(nullptr);
            }

            _writer.EndObject();   // element {}
        }
        _writer.EndArray();   // typeName []
    }

    _writer.EndObject();   // components {}
    _writer.EndObject();   // root {}
}

Error Scene::Deserialize(
    ArchiveReader&                _reader,
    Flags<eSceneDeserializeFlags> _flags)
{
    Reset_();

    // scene type
    StringView sceneType = {};
    if (const Error err = _reader.ReadField("sceneType", sceneType))
    {
        JAM_LOG_ERROR("Scene deserialize error: failed to read 'sceneType' field.\n{}", err);
        return err;
    }

    if (GetTypeName() != sceneType)
    {
        if (_flags & eSceneDeserializeFlags::SkipSceneTypeMismatch)
        {
            JAM_LOG_WARN("Scene deserialize warning: scene type mismatch. "
                         "The serialized data has scene type '{}', but the current scene has type '{}'. "
                         "The deserializer will attempt to deserialize the data anyway, but this may lead to "
                         "unexpected behavior or errors if the scene types are not compatible. "
                         "Please ensure that the serialized data is intended for the current scene type.",
                         sceneType,
                         GetTypeName());
        }
        else
        {
            JAM_LOG_ERROR("Scene deserialize error: scene type mismatch. "
                          "The serialized data has scene type '{}', but the current scene has type '{}'. "
                          "The deserializer will skip deserialization to prevent potential issues. "
                          "Please ensure that the serialized data is intended for the current scene type.",
                          sceneType,
                          GetTypeName());

            return Error::TypeMismatch;
        }
    }

    // entities
    if (const Error err = _reader.EnterField("entities"))
    {
        JAM_LOG_ERROR("Scene deserialize error: failed to enter 'entities' field.\n{}", err);
        return err;
    }

    const UID rootUid = GetRootEntity().GetUID();
    if (const Error err = _reader.ForEachArray(Bind([this, rootUid](const SizeT _index, ArchiveReader& _reader) -> Error {
            return DeserializeEntity_(*this, rootUid, _index, _reader);
        })))
    {
        JAM_LOG_ERROR("Scene deserialize error: failed to read entities array.\n{}", err);
        return err;
    }
    _reader.Exit();

    // components
    if (const Error err = _reader.EnterField("components"))
    {
        JAM_LOG_ERROR("Scene deserialize error: failed to enter 'components' field.\n{}", err);
        return err;
    }

    if (const Error err = _reader.ForEachObject(Bind([this, _flags](const StringView _key, ArchiveReader& _reader) {
            return DeserializeComponent_(*this, _flags, _key, _reader);
        })))
    {
        JAM_LOG_ERROR("Scene deserialize error: failed to read components.\n{}", err);
        _reader.Exit();
        return err;
    }
    _reader.Exit();

    return Error::None;
}

}   // namespace Jam