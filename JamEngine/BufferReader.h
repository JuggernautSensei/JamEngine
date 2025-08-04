#pragma once
#include "Buffers.h"

namespace jam
{

class BufferReader
{
public:
    void      ReserveBuffer(UInt32 _byteWidth);
    NODISCARD std::optional<std::vector<UInt8>> ReadBuffer(ID3D11Buffer* _pBuffer);
    NODISCARD std::optional<std::vector<UInt8>> ReadBuffer(const Buffer& _buffer);

private:
    NODISCARD std::optional<std::vector<UInt8>> ReadBuffer_(ID3D11Buffer* _pBuffer, UInt32 _bufferByteWidth);

    StagingBuffer m_stagingBuffer;   // staging buffer for reading
};

}   // namespace jam