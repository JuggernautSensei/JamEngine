#pragma once

namespace jam
{

class ShaderProgram
{
public:
    void Initialize(ID3DBlob* _pVScodeOrNull = nullptr,
                    ID3DBlob* _pPScodeOrNull = nullptr,
                    ID3DBlob* _pGScodeOrNull = nullptr,
                    ID3DBlob* _pHScodeOrNull = nullptr,
                    ID3DBlob* _pDScodeOrNull = nullptr);

    void Bind() const;

    // accessor
    NODISCARD ID3D11InputLayout*    GetInputLayout() const { return m_pInputLayout.Get(); }
    NODISCARD ID3D11VertexShader*   GetVertexShader() const { return m_pVertexShader.Get(); }
    NODISCARD ID3D11PixelShader*    GetPixelShader() const { return m_pPixelShader.Get(); }
    NODISCARD ID3D11GeometryShader* GetGeometryShader() const { return m_pGeometryShader.Get(); }
    NODISCARD ID3D11HullShader*     GetHullShader() const { return m_pHullShader.Get(); }
    NODISCARD ID3D11DomainShader*   GetDomainShader() const { return m_pDomainShader.Get(); }

private:
    void CreateInputLayout_(ID3DBlob* _pVSCode);

    ComPtr<ID3D11InputLayout>    m_pInputLayout;
    ComPtr<ID3D11VertexShader>   m_pVertexShader;
    ComPtr<ID3D11PixelShader>    m_pPixelShader;
    ComPtr<ID3D11GeometryShader> m_pGeometryShader;
    ComPtr<ID3D11HullShader>     m_pHullShader;
    ComPtr<ID3D11DomainShader>   m_pDomainShader;
};

}   // namespace jam