#include "pch.h"

#include "Components.h"

namespace
{
using namespace jam;

bool DrawVector3Editor(const char* _label, Vec3& _ref_vec, const float _speed, const float _minValue, const float _maxValue, const Vec3& _resetValue, const char* _format = "%.3f")
{
    ImGuiStyle& style = ImGui::GetStyle();
    bool        dirty = false;

    ImGui::PushID(_label);

    float  availWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 buttonSize = ImVec2 { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
    float  itemWidth  = (availWidth - buttonSize.x * 3.f - style.ItemInnerSpacing.x * 5.f) / 3.f;

    // |x|drag|y|drag|z|drag|

    // x
    if (ImGui::Button("X", buttonSize))
    {
        _ref_vec.x = _resetValue.x;
        dirty      = true;
    }
    ImGui::SameLine(style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##x", &_ref_vec.x, _speed, _minValue, _maxValue, _format, ImGuiSliderFlags_WrapAround))
    {
        dirty = true;
    }

    // y
    ImGui::SameLine(style.ItemInnerSpacing.x);
    if (ImGui::Button("Y", buttonSize))
    {
        _ref_vec.y = _resetValue.y;
        dirty      = true;
    }
    ImGui::SameLine(style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##y", &_ref_vec.y, _speed, _minValue, _maxValue, _format, ImGuiSliderFlags_WrapAround))
    {
        dirty = true;
    }

    // z
    ImGui::SameLine(style.ItemInnerSpacing.x);
    if (ImGui::Button("Z", buttonSize))
    {
        _ref_vec.z = _resetValue.z;
        dirty      = true;
    }
    ImGui::SameLine(style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##z", &_ref_vec.z, _speed, _minValue, _maxValue, _format, ImGuiSliderFlags_WrapAround))
    {
        dirty = true;
    }

    ImGui::PopID();
    return dirty;
}

}   // namespace

namespace jam
{

void TagComponent::DrawComponentEditor(const DrawComponentEditorParameter& _param)
{
    if (ImGui::BeginTable("tag component", 2))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("tag");

        ImGui::TableSetColumnIndex(1);
        ImGui::InputText("##tag", &name);
        ImGui::EndTable();
    }
}

void TransformComponent::DrawComponentEditor(const DrawComponentEditorParameter& _param)
{
    if (ImGui::BeginTable("transform component", 2))
    {
        // 위치
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("position");

            ImGui::TableSetColumnIndex(1);
            DrawVector3Editor("##position", position, 0.1f, 0.f, 0.f, Vec3::Zero);
        }

        // 회전
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("rotation");

            ImGui::TableSetColumnIndex(1);
            Vec3 preEuler  = ToDeg(rotation.ToEuler());
            Vec3 postEuler = preEuler;
            if (DrawVector3Editor("##rotation", postEuler, 0.1f, -180.f, 180.f, Vec3::Zero, "%.3f deg"))
            {
                Vec3 delta = ToRad(postEuler - preEuler);
                Quat q     = Quat::CreateFromYawPitchRoll(delta.y, delta.x, delta.z);
                rotation   = q * rotation; // 곱셈 순서 확인
            }
        }

        // 스케일
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("scale");

            ImGui::TableSetColumnIndex(1);
            DrawVector3Editor("##scale", scale, 0.1f, 0.f, 0.f, Vec3::One);
        }

        ImGui::EndTable();
    }
}
}

}   // namespace jam