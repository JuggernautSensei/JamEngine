#pragma once

[[nodiscard]] ShaderHandle CompileShader(const std::string_view _source, const eShader _stage, const std::string_view _entryPoint);