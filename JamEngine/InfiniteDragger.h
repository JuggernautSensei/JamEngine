#pragma once

namespace Jam
{

// use like this:
// ...
// InfiniteDragger dragger(eInfiniteDraggerFlags::Left | eInfiniteDraggerFlags::Right);
// dragger.Update_();
// Vec2i delta = dragger.GetDragDelta();
// ...

enum class eInfiniteDraggerFlags : UInt32
{
    None   = 0,
    Left   = 1 << 0,   // left mouse button trigger
    Right  = 1 << 1,   // right mouse button trigger
    Middle = 1 << 2,   // middle mouse button trigger

    // FixCursorAtCenterOnDragging = 1 << 3,   // default behavior is to fix cursor to captured position.
    // NotFixCursor = 1 << 4,                  // do not fix cursor position when dragging. just accumulate delta.
};

class InfiniteDragger
{
public:
    explicit InfiniteDragger(Flags<eInfiniteDraggerFlags> _buttonFlags = eInfiniteDraggerFlags::Left);

    void                Update();
    [[nodiscard]] Vec2i GetDragDelta() const;

private:
    Flags<eInfiniteDraggerFlags> m_buttonFlags = eInfiniteDraggerFlags::None;

    Vec2i m_capturedPos = Vec2i::k_zero;
    Vec2i m_deltaPos    = Vec2i::k_zero;
    bool  m_bIsDragging = false;
};

}   // namespace Jam
