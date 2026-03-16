#include "pch.h"
#include "Events.h"
#include "Resources.h"
#include "FileBrowserWindow.h"

ProjectChangedEvent::ProjectChangedEvent()
{
}

String ProjectChangedEvent::ToString() const
{
    return "ProjectChangedEvent";
}

FocusingShaderItemChangedEvent::FocusingShaderItemChangedEvent()
{
}

String FocusingShaderItemChangedEvent::ToString() const
{
    return "FocusingShaderItemChangedEvent";
}

ProjectRenamedEvent::ProjectRenamedEvent()
{
}

String ProjectRenamedEvent::ToString() const
{
    return "ProjectRenamedEvent";
}

ProjectDirtyChangedEvent::ProjectDirtyChangedEvent()
{
}

String ProjectDirtyChangedEvent::ToString() const
{
    return "ProjectDirtyChangedEvent";
}
