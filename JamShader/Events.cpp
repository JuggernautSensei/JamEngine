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

FocusingShaderItemChagnedEvent::FocusingShaderItemChagnedEvent()
{
}

String FocusingShaderItemChagnedEvent::ToString() const
{
    return "FocusingShaderItemChagnedEvent";
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
