#include "pch.h"
#include "InfiniteDragger.h"

#include "Input.h"

namespace Jam
{

InfiniteDragger::InfiniteDragger(
    const Flags<eInfiniteDraggerFlags> _buttonFlags)
    : m_buttonFlags(_buttonFlags)
{
}

void InfiniteDragger::Update()
{
    Input& input = GetInput();

    // check drag start
    if (!m_bIsDragging)
    {
        const bool bLeft   = (m_buttonFlags & eInfiniteDraggerFlags::Left) && input.IsMousePressed(eMouse::LeftButton);
        const bool bRight  = (m_buttonFlags & eInfiniteDraggerFlags::Right) && input.IsMousePressed(eMouse::RightButton);
        const bool bMiddle = (m_buttonFlags & eInfiniteDraggerFlags::Middle) && input.IsMousePressed(eMouse::MiddleButton);

        if (bLeft || bRight || bMiddle)
        {
            m_bIsDragging = true;
            m_capturedPos = input.GetMousePos();
            m_deltaPos    = Vec2i::kZero;
            input.SetMouseVisibility(false);
        }
    }
    else
    {
        // during dragging
        m_deltaPos = input.GetMousePos() - m_capturedPos;

        // fix mouse pos to captured pos
        if (m_deltaPos.x != 0 || m_deltaPos.y != 0)
        {
            input.SetMousePos(m_capturedPos.x, m_capturedPos.y);
        }

        // check drag end
        const bool bLeft   = !(m_buttonFlags & eInfiniteDraggerFlags::Left) || input.IsMouseUp(eMouse::LeftButton);
        const bool bRight  = !(m_buttonFlags & eInfiniteDraggerFlags::Right) || input.IsMouseUp(eMouse::RightButton);
        const bool bMiddle = !(m_buttonFlags & eInfiniteDraggerFlags::Middle) || input.IsMouseUp(eMouse::MiddleButton);

        // all flagged buttons are up -> end drag
        if (bLeft && bRight && bMiddle)
        {
            m_bIsDragging = false;
            m_deltaPos    = Vec2i::kZero;
            m_capturedPos = Vec2i::kZero;
            input.SetMouseVisibility(true);
        }
    }
}

Vec2i InfiniteDragger::GetDragDelta() const
{
    return m_deltaPos;
}

}   // namespace Jam