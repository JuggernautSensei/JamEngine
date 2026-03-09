#include "pch.h"
#include "GUIAssetManager.h"

#include "ResourceManager.h"
#include "ShaderApp.h"

namespace GUI
{
namespace AssetManager
{

    Expect<Ref<ShaderSource>> LoadShaderSource(
        const StringView _path,
        const bool       _bReload)
    {
        const Expect<Ref<ShaderSource>> pSource = GetResourceManager().LoadShaderSource(_path, _bReload);
        if (!pSource)
        {
            GetShaderApp().PushMessageDialog(
                "failed to load shader source",
                String::Formatted("failed to load shader source from path \"{}\". error: {}", _path, pSource.GetError().ToErrorMessage()));

            return pSource.GetError();
        }

        return pSource;
    }

}   // namespace AssetManager
}   // namespace GUI