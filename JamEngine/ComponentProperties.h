#pragma once
#include "Entity.h"

namespace Jam
{

void foo()
{
    entt::handle
}
/*

[SerializableComponent]

if you want a component to be serializable, it must implement the following methods:

struct SampleComponent
{
    void Serialize(
            ArchiveWriter&    _writer,
            const Ref<Scene>& _pScene,
            const Entity      _owner) const (const or non-const. both allowed)
    {
            _writer.Write(data);
            // serialize other fields...
    }

    void Deserialize(
            ArchiveReader&    _reader,
            const Ref<Scene>& _pScene,
            const Entity      _owner)
    {
            _reader.Read(data);
            // deserialize other fields...
    }

    SomeData data;
};

*/

template<typename TCmp>
concept SerializableComponent =   // serialize is const or non-const (both)
    requires(                     // deserialize must be non-const
        TCmp&             _cmp,
        ArchiveWriter&    _writer,
        ArchiveReader&    _reader,
        const Ref<Scene>& _pScene,
        const Entity      _owner) {
        { _cmp.Serialize(_writer, _pScene, _owner) } -> std::same_as<void>;
        { _cmp.Deserialize(_reader, _pScene, _owner) } -> std::same_as<void>;

        requires !requires(const TCmp& _ccmp) {
            { _ccmp.Deserialize(_reader, _pScene, _owner) } -> std::same_as<void>;
        };
    };

#define JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(TCmp) \
    static_assert(SerializableComponent<TCmp>, "Component '" #TCmp "' must implement Serialize and Deserialize methods to be serializable.");

/*

[EditableComponent]

if you want a component to be editable in the editor, it must implement the following method:

struct SampleComponent
{
    void DrawEditPanel(
            const Ref<Scene>& _pScene,
            const Entity      _owner)
    {
            // draw UI elements for editing component properties
            ImGui::InputFloat("Some Value", &someValue);
            // draw other properties...
    }

    float someValue = 0.f;
};

*/

template<typename TCmp>
concept EditableComponent =   // DrawEditPanel must be non-const
    requires(
        TCmp&             _cmp,
        const Ref<Scene>& _pScene,
        const Entity      _owner) {
        { _cmp.DrawEditPanel(_pScene, _owner) } -> std::same_as<void>;

        requires !requires(const TCmp& _ccmp) {
            { _ccmp.DrawEditPanel(_pScene, _owner) } -> std::same_as<void>;
        };
    };

#define JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(TCmp) \
    static_assert(EditableComponent<TCmp>, "Component '" #TCmp "' must implement DrawEditPanel method to be editable in the editor.");

}   // namespace Jam