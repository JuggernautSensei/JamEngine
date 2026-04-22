#include "pch.h"

#include "ShaderExporter.h"

#include "StringUtilities.h"
#include "WindowsUtilities.h"

#include <d3dcompiler.h>

namespace jam
{

eErrorCode ShaderExporter::Export()
{
    JAM_ASSERT(!m_filePath.empty(), "File path is not set.");
    JAM_ASSERT(IsExportablePath(m_filePath), "File path is not exportable. File path: '{}'", m_filepath.native());
    JAM_ASSERT(m_pBlob != nullptr, "Shader bytecode is not set.");

    std::wstring wPath = m_filePath.wstring();
    HRESULT      hr    = D3DWriteBlobToFile(m_pBlob.Get(), wPath.c_str(), TRUE);
    if (FAILED(hr))
    {
        SetErrorString("Failed to write shader bytecode to file: '{}'", m_filePath.native());
        return eErrorCode::FileWriteFailed;
    }
    return eErrorCode::Ok;
}

void ShaderExporter::SetPath(
    const fs::path& _filePath)
{
    m_filePath = _filePath;
}

void ShaderExporter::SetShaderBytecode(const MemoryView& _shaderBytecode)
{
    const Byte* pData     = _shaderBytecode.GetDataPtr();
    size_t      byteWidth = _shaderBytecode.GetByteWidth();
    JAM_CHECK_COM(D3DCreateBlob(byteWidth, &m_pBlob));
    std::memcpy(m_pBlob->GetBufferPointer(), pData, byteWidth);
}

void ShaderExporter::Reset()
{
    m_filePath.clear();
    m_pBlob.Reset();
    m_bAllowOverwrite = false;
}

bool ShaderExporter::IsExportablePath(
    const fs::path& _filePath) const
{
    fs::path    ext      = _filePath.extension();
    std::string lowerExt = ToLower(ext.native());
    return lowerExt == ".cso";
}

}   // namespace jam