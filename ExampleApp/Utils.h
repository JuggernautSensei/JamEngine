#pragma once

JAM_NODISCARD ShaderHandle CompileShader(const std::string_view _source, const eShader _stage, const std::string_view _entryPoint);