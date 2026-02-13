#pragma once

namespace Jam
{

class Scene;

enum class eSceneDeserializeMode : UInt32
{
    None,
    RemoveComponentOnDeserializeFailure,   // invalid component is destroyed during deserialization.
                                           // if you not set this flag, invalid component deserialization will cause an whole error.
};

void               SerializeScene(ArchiveWriter& _writer, const Ref<Scene>& _pScene);
Expect<Ref<Scene>> DeserializeScene(ArchiveReader& _reader, Flags<eSceneDeserializeMode> _flags = {});

}   // namespace Jam
