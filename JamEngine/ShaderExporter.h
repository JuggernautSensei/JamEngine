#pragma once
#include "IExporter.h"
#include "Memory.h"

namespace jam
{

class ShaderExporter : public IExporter
{
public:
    eErrorCode Export() override;

    void SetPath(const fs::path& _filePath) override;
    void SetAllowOverwrite(const bool _bAllow) { m_bAllowOverwrite = _bAllow; }
    void SetShaderBytecode(const MemoryView& _shaderBytecode);
    void Reset() override;

    JAM_NODISCARD bool IsExportablePath(const fs::path& _filePath) const override;

private:
    fs::path         m_filePath        = {};
    bool             m_bAllowOverwrite = false;
    ComPtr<ID3DBlob> m_pBlob           = {};
};

}   // namespace jam
