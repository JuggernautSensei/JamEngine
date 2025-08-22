#include "pch.h"

#include "ShaderProgram.h"

#include "Renderer.h"
#include "WindowsUtilities.h"
#include <d3dcompiler.h>

namespace jam
{

void ShaderProgram::Initialize(ID3DBlob* _pVScodeOrNull, ID3DBlob* _pPScodeOrNull, ID3DBlob* _pGScodeOrNull, ID3DBlob* _pHScodeOrNull, ID3DBlob* _pDScodeOrNull)
{
    if (_pVScodeOrNull)
    {
        ShaderCreateInfo data;
        data.pBytecode      = _pVScodeOrNull->GetBufferPointer();
        data.bytecodeLength = _pVScodeOrNull->GetBufferSize();

        Renderer::CreateVertexShader(data, m_pVertexShader.GetAddressOf());
        CreateInputLayout_(_pVScodeOrNull);   // 인풋 레이아웃 생성
    }

    if (_pPScodeOrNull)
    {
        ShaderCreateInfo data;
        data.pBytecode      = _pPScodeOrNull->GetBufferPointer();
        data.bytecodeLength = _pPScodeOrNull->GetBufferSize();

        Renderer::CreatePixelShader(data, m_pPixelShader.GetAddressOf());
    }

    if (_pGScodeOrNull)
    {
        ShaderCreateInfo data;
        data.pBytecode      = _pGScodeOrNull->GetBufferPointer();
        data.bytecodeLength = _pGScodeOrNull->GetBufferSize();

        Renderer::CreateGeometryShader(data, m_pGeometryShader.GetAddressOf());
    }

    if (_pHScodeOrNull)
    {
        ShaderCreateInfo data;
        data.pBytecode      = _pHScodeOrNull->GetBufferPointer();
        data.bytecodeLength = _pHScodeOrNull->GetBufferSize();

        Renderer::CreateHullShader(data, m_pHullShader.GetAddressOf());
    }

    if (_pDScodeOrNull)
    {
        ShaderCreateInfo data;
        data.pBytecode      = _pDScodeOrNull->GetBufferPointer();
        data.bytecodeLength = _pDScodeOrNull->GetBufferSize();

        Renderer::CreateDomainShader(data, m_pDomainShader.GetAddressOf());
    }
}

void ShaderProgram::Bind() const
{
    Renderer::BindInputLayout(m_pInputLayout.Get());
    Renderer::BindVertexShader(m_pVertexShader.Get());
    Renderer::BindPixelShader(m_pPixelShader.Get());
    Renderer::BindGeometryShader(m_pGeometryShader.Get());
    Renderer::BindHullShader(m_pHullShader.Get());
    Renderer::BindDomainShader(m_pDomainShader.Get());
}

void ShaderProgram::CreateInputLayout_(ID3DBlob* _pVSCode)
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> elems;

    // 리플랙션 데이터
    HRESULT                        hr;
    ComPtr<ID3D11ShaderReflection> pShader;
    hr = D3DReflect(_pVSCode->GetBufferPointer(), _pVSCode->GetBufferSize(), IID_PPV_ARGS(&pShader));
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to reflect shader code. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    // 정보 추출
    D3D11_SHADER_DESC desc;
    hr = pShader->GetDesc(&desc);

    if (FAILED(hr))
    {
        JAM_CRASH("Failed to get shader description. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    elems.reserve(desc.InputParameters);
    for (UINT i = 0; i < desc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        hr = pShader->GetInputParameterDesc(i, &paramDesc);

        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get input parameter description. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        // desc 생성
        D3D11_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName             = paramDesc.SemanticName;
        elementDesc.SemanticIndex            = paramDesc.SemanticIndex;

        // 포맷 결정
        if (paramDesc.Mask == 1)   // 단일 float
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32_UINT;
        }
        else if (paramDesc.Mask <= 3)   // float2
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
        }
        else if (paramDesc.Mask <= 7)   // float3
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
        }
        else if (paramDesc.Mask <= 15)   // float4
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
        }

        elementDesc.InputSlot            = 0;   // 단일 버텍스 슬롯만 사용
        elementDesc.AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        elems.emplace_back(elementDesc);
    }

    // input layouy 생성
    ID3D11Device* pDevice = Renderer::GetDevice();
    hr                    = pDevice->CreateInputLayout(elems.data(),
                                    static_cast<UINT>(elems.size()),
                                    _pVSCode->GetBufferPointer(),
                                    _pVSCode->GetBufferSize(),
                                    m_pInputLayout.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create input layout. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

}   // namespace jam