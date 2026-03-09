#pragma once

namespace GUI
{
namespace AssetManager
{

    Expect<Ref<ShaderSource>> LoadShaderSource(StringView _path, bool _bReload = false);

}
}   // namespace GUI