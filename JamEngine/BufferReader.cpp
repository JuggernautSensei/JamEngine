#include "pch.h"
#include "BufferReader.h"

#include "Renderer.h"

namespace jam
{

void BufferReader::ReserveBuffer(const UInt32 _byteWidth)
{
    if (m_stagingBuffer.GetByteWidth() < _byteWidth)
    {
        m_stagingBuffer.Initialize(_byteWidth);
    }
}

Result<std::vector<UInt8>> BufferReader::ReadBuffer(ID3D11Buffer* _pBuffer)
{
    JAM_ASSERT(_pBuffer != nullptr, "BufferReader::ReadBuffer - Invalid buffer pointer.");
    D3D11_BUFFER_DESC desc;
    _pBuffer->GetDesc(&desc);
    return ReadBuffer_(_pBuffer, desc.ByteWidth);
}

Result<std::vector<UInt8>> BufferReader::ReadBuffer(const Buffer& _buffer)
{
    return ReadBuffer_(_buffer.Get(), _buffer.GetByteWidth());
}

Result<std::vector<UInt8>> BufferReader::ReadBuffer_(ID3D11Buffer* _pBuffer, const UInt32 _bufferByteWidth)
{
    JAM_ASSERT(_pBuffer != nullptr, "BufferReader::ReadBuffer_ - Invalid buffer pointer.");

    // reallocate
    if (m_stagingBuffer.GetByteWidth() < _bufferByteWidth)
    {
        m_stagingBuffer.Initialize(_bufferByteWidth);
    }

    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    ctx->CopySubresourceRegion(m_stagingBuffer.Get(), 0, 0, 0, 0, _pBuffer, 0, nullptr);
    return m_stagingBuffer.ReadData();
}

}