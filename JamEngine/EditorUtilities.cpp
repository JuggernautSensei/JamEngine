#include "pch.h"

#include "EditorUtilities.h"

#include "Editor.h"
#include "Graphics.h"
#include "SceneManager.h"

namespace Jam
{

bool IsZeroApprox(
    const ImVec2 _vec)
{
    return IsZeroApprox(_vec.x) && IsZeroApprox(_vec.y);
}

ImVec4 ImVec4FromHex(
    const UInt32 _rgba)
{
    return ImVec4 {
        NumericCast<float>((_rgba >> 24) & 0xFF) / 255.f,
        NumericCast<float>((_rgba >> 16) & 0xFF) / 255.f,
        NumericCast<float>((_rgba >> 8) & 0xFF) / 255.f,
        NumericCast<float>(_rgba & 0xFF) / 255.f
    };
}

ImVec2 ImVec2From(
    const Vec2i _vec)
{
    return ImVec2 { NumericCast<float>(_vec.x), NumericCast<float>(_vec.y) };
}

ImVec2 CalcFitButtonSize(
    const Int32 _numButtons,
    ImVec2      _region)
{
    if (_region == ImVec2 { 0.f, 0.f })
    {
        _region   = ImGui::GetContentRegionAvail();
        _region.y = 0.f;
    }

    if (_numButtons == 1)
    {
        return _region;
    }

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalW  = _region.x - spacing * NumericCast<float>(_numButtons - 1);
    const float buttonW = totalW / NumericCast<float>(_numButtons);
    return ImVec2 { buttonW, _region.y };
}

void SetNextItemToAvailWidth()
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

ImVec2 GetContentRegionAvailScaled(
    const float _sx,
    const float _sy)
{
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    return { avail.x * _sx, avail.y * _sy };
}

enum class eEditorPayload
{
    File = 0,
    Entity,
};

const char* EditorPayloadFrom(
    const eEditorPayload _payload)
{
    switch (_payload)
    {
        case eEditorPayload::File:
            return "JAM_EDITOR_PAYLOAD_FILE";
        case eEditorPayload::Entity:
            return "JAM_EDITOR_PAYLOAD_ENTITY";
        default:
            JAM_UNREACHABLE;
    }
}

bool SetFileDragDropPayload(
    const Path& _path)
{
    const std::string& path = _path.native();
    const char*        str  = path.c_str();
    const USizeT       len  = path.size();
    return ImGui::SetDragDropPayload(EditorPayloadFrom(eEditorPayload::File), str, len);
}

const ImGuiPayload* AcceptFileDragDropPayload()
{
    return ImGui::AcceptDragDropPayload(EditorPayloadFrom(eEditorPayload::File));
}

Path ResolvePathDragDropPayload(
    const ImGuiPayload* _pPayload)
{
    JAM_ASSERT(_pPayload, "payload is nullptr");
    JAM_ASSERT(_pPayload->IsDataType(EditorPayloadFrom(eEditorPayload::File)), "payload type mismatch");
    const char* path = static_cast<const char*>(_pPayload->Data);
    const Int32 len  = _pPayload->DataSize;
    return std::string { path, NumericCast<USizeT>(len) };
}

bool SetEntityDragDropPayload(
    const Entity _entity)
{
    const UID uid = _entity.GetUID();
    return ImGui::SetDragDropPayload(EditorPayloadFrom(eEditorPayload::Entity), &uid, sizeof(UID));
}

const ImGuiPayload* AcceptEntityDragDropPayload()
{
    return ImGui::AcceptDragDropPayload(EditorPayloadFrom(eEditorPayload::Entity));
}

Entity ResolveEntityDragDropPayload(
    const ImGuiPayload* _pPayload)
{
    JAM_ASSERT(_pPayload, "payload is nullptr");
    JAM_ASSERT(_pPayload->IsDataType(EditorPayloadFrom(eEditorPayload::Entity)), "payload type mismatch");
    JAM_ASSERT(JAM_SCENE_MGR.HasActiveScene(), "There is no active scene");

    const UID uid = *static_cast<const UID*>(_pPayload->Data);
    return JAM_SCENE_MGR.GetActiveScene()->GetEntity(uid);
}

StringView EditorDisplayNameOf(
    const ImGuiKey _key)
{
    switch (_key)
    {
        case ImGuiKey_None:
            return "none";
        case ImGuiKey_Tab:
            return "tab";
        case ImGuiKey_LeftArrow:
            return "left arrow";
        case ImGuiKey_RightArrow:
            return "right arrow";
        case ImGuiKey_UpArrow:
            return "up arrow";
        case ImGuiKey_DownArrow:
            return "down arrow";
        case ImGuiKey_PageUp:
            return "page up";
        case ImGuiKey_PageDown:
            return "page down";
        case ImGuiKey_Home:
            return "home";
        case ImGuiKey_End:
            return "end";
        case ImGuiKey_Insert:
            return "insert";
        case ImGuiKey_Delete:
            return "delete";
        case ImGuiKey_Backspace:
            return "backspace";
        case ImGuiKey_Space:
            return "space";
        case ImGuiKey_Enter:
            return "enter";
        case ImGuiKey_Escape:
            return "escape";
        case ImGuiKey_LeftCtrl:
            return "left ctrl";
        case ImGuiKey_LeftShift:
            return "left shift";
        case ImGuiKey_LeftAlt:
            return "left alt";
        case ImGuiKey_LeftSuper:
            return "left super";
        case ImGuiKey_RightCtrl:
            return "right ctrl";
        case ImGuiKey_RightShift:
            return "right shift";
        case ImGuiKey_RightAlt:
            return "right alt";
        case ImGuiKey_RightSuper:
            return "right super";
        case ImGuiKey_Menu:
            return "menu";
        case ImGuiKey_0:
            return "0";
        case ImGuiKey_1:
            return "1";
        case ImGuiKey_2:
            return "2";
        case ImGuiKey_3:
            return "3";
        case ImGuiKey_4:
            return "4";
        case ImGuiKey_5:
            return "5";
        case ImGuiKey_6:
            return "6";
        case ImGuiKey_7:
            return "7";
        case ImGuiKey_8:
            return "8";
        case ImGuiKey_9:
            return "9";
        case ImGuiKey_A:
            return "a";
        case ImGuiKey_B:
            return "b";
        case ImGuiKey_C:
            return "c";
        case ImGuiKey_D:
            return "d";
        case ImGuiKey_E:
            return "e";
        case ImGuiKey_F:
            return "f";
        case ImGuiKey_G:
            return "g";
        case ImGuiKey_H:
            return "h";
        case ImGuiKey_I:
            return "i";
        case ImGuiKey_J:
            return "j";
        case ImGuiKey_K:
            return "k";
        case ImGuiKey_L:
            return "l";
        case ImGuiKey_M:
            return "m";
        case ImGuiKey_N:
            return "n";
        case ImGuiKey_O:
            return "o";
        case ImGuiKey_P:
            return "p";
        case ImGuiKey_Q:
            return "q";
        case ImGuiKey_R:
            return "r";
        case ImGuiKey_S:
            return "s";
        case ImGuiKey_T:
            return "t";
        case ImGuiKey_U:
            return "u";
        case ImGuiKey_V:
            return "v";
        case ImGuiKey_W:
            return "w";
        case ImGuiKey_X:
            return "x";
        case ImGuiKey_Y:
            return "y";
        case ImGuiKey_Z:
            return "z";
        case ImGuiKey_F1:
            return "f1";
        case ImGuiKey_F2:
            return "f2";
        case ImGuiKey_F3:
            return "f3";
        case ImGuiKey_F4:
            return "f4";
        case ImGuiKey_F5:
            return "f5";
        case ImGuiKey_F6:
            return "f6";
        case ImGuiKey_F7:
            return "f7";
        case ImGuiKey_F8:
            return "f8";
        case ImGuiKey_F9:
            return "f9";
        case ImGuiKey_F10:
            return "f10";
        case ImGuiKey_F11:
            return "f11";
        case ImGuiKey_F12:
            return "f12";
        case ImGuiKey_F13:
            return "f13";
        case ImGuiKey_F14:
            return "f14";
        case ImGuiKey_F15:
            return "f15";
        case ImGuiKey_F16:
            return "f16";
        case ImGuiKey_F17:
            return "f17";
        case ImGuiKey_F18:
            return "f18";
        case ImGuiKey_F19:
            return "f19";
        case ImGuiKey_F20:
            return "f20";
        case ImGuiKey_F21:
            return "f21";
        case ImGuiKey_F22:
            return "f22";
        case ImGuiKey_F23:
            return "f23";
        case ImGuiKey_F24:
            return "f24";
        case ImGuiKey_Apostrophe:
            return "apostrophe";
        case ImGuiKey_Comma:
            return "comma";
        case ImGuiKey_Minus:
            return "minus";
        case ImGuiKey_Period:
            return "period";
        case ImGuiKey_Slash:
            return "slash";
        case ImGuiKey_Semicolon:
            return "semicolon";
        case ImGuiKey_Equal:
            return "equal";
        case ImGuiKey_LeftBracket:
            return "left bracket";
        case ImGuiKey_Backslash:
            return "backslash";
        case ImGuiKey_RightBracket:
            return "right bracket";
        case ImGuiKey_GraveAccent:
            return "grave accent";
        case ImGuiKey_CapsLock:
            return "caps lock";
        case ImGuiKey_ScrollLock:
            return "scroll lock";
        case ImGuiKey_NumLock:
            return "num lock";
        case ImGuiKey_PrintScreen:
            return "print screen";
        case ImGuiKey_Pause:
            return "pause";
        case ImGuiKey_Keypad0:
            return "keypad 0";
        case ImGuiKey_Keypad1:
            return "keypad 1";
        case ImGuiKey_Keypad2:
            return "keypad 2";
        case ImGuiKey_Keypad3:
            return "keypad 3";
        case ImGuiKey_Keypad4:
            return "keypad 4";
        case ImGuiKey_Keypad5:
            return "keypad 5";
        case ImGuiKey_Keypad6:
            return "keypad 6";
        case ImGuiKey_Keypad7:
            return "keypad 7";
        case ImGuiKey_Keypad8:
            return "keypad 8";
        case ImGuiKey_Keypad9:
            return "keypad 9";
        case ImGuiKey_KeypadDecimal:
            return "keypad decimal";
        case ImGuiKey_KeypadDivide:
            return "keypad divide";
        case ImGuiKey_KeypadMultiply:
            return "keypad multiply";
        case ImGuiKey_KeypadSubtract:
            return "keypad subtract";
        case ImGuiKey_KeypadAdd:
            return "keypad add";
        case ImGuiKey_KeypadEnter:
            return "keypad enter";
        case ImGuiKey_KeypadEqual:
            return "keypad equal";
        case ImGuiKey_AppBack:
            return "app back";
        case ImGuiKey_AppForward:
            return "app forward";
        case ImGuiKey_Oem102:
            return "oem 102";
        case ImGuiKey_GamepadStart:
            return "gamepad start";
        case ImGuiKey_GamepadBack:
            return "gamepad back";
        case ImGuiKey_GamepadFaceLeft:
            return "gamepad face left";
        case ImGuiKey_GamepadFaceRight:
            return "gamepad face right";
        case ImGuiKey_GamepadFaceUp:
            return "gamepad face up";
        case ImGuiKey_GamepadFaceDown:
            return "gamepad face down";
        case ImGuiKey_GamepadDpadLeft:
            return "gamepad dpad left";
        case ImGuiKey_GamepadDpadRight:
            return "gamepad dpad right";
        case ImGuiKey_GamepadDpadUp:
            return "gamepad dpad up";
        case ImGuiKey_GamepadDpadDown:
            return "gamepad dpad down";
        case ImGuiKey_GamepadL1:
            return "gamepad l1";
        case ImGuiKey_GamepadR1:
            return "gamepad r1";
        case ImGuiKey_GamepadL2:
            return "gamepad l2";
        case ImGuiKey_GamepadR2:
            return "gamepad r2";
        case ImGuiKey_GamepadL3:
            return "gamepad l3";
        case ImGuiKey_GamepadR3:
            return "gamepad r3";
        case ImGuiKey_GamepadLStickLeft:
            return "gamepad l stick left";
        case ImGuiKey_GamepadLStickRight:
            return "gamepad l stick right";
        case ImGuiKey_GamepadLStickUp:
            return "gamepad l stick up";
        case ImGuiKey_GamepadLStickDown:
            return "gamepad l stick down";
        case ImGuiKey_GamepadRStickLeft:
            return "gamepad r stick left";
        case ImGuiKey_GamepadRStickRight:
            return "gamepad r stick right";
        case ImGuiKey_GamepadRStickUp:
            return "gamepad r stick up";
        case ImGuiKey_GamepadRStickDown:
            return "gamepad r stick down";
        case ImGuiKey_MouseLeft:
            return "mouse left";
        case ImGuiKey_MouseRight:
            return "mouse right";
        case ImGuiKey_MouseMiddle:
            return "mouse middle";
        case ImGuiKey_MouseX1:
            return "mouse x1";
        case ImGuiKey_MouseX2:
            return "mouse x2";
        case ImGuiKey_MouseWheelX:
            return "mouse wheel x";
        case ImGuiKey_MouseWheelY:
            return "mouse wheel y";
        case ImGuiMod_Ctrl:
            return "ctrl";
        case ImGuiMod_Shift:
            return "shift";
        case ImGuiMod_Alt:
            return "alt";
        case ImGuiMod_Super:
            return "super";
        case ImGuiMod_Mask_:
            return "mask";
        default: return "unknown";
    }
}

bool BeginLabelTable(const char* _label)
{
    if (ImGui::BeginTable(_label, 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    return false;
}

}   // namespace Jam