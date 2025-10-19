#pragma once
#pragma once
#include "GfxBase.h"

/////////////////////////////
///          FWD          ///
/////////////////////////////

// clang-format off
namespace DirectX { class ScratchImage; }
namespace jam { class MemoryView; class Mesh; class Event; }
// clang-format on

//////////////////////////////
///          CORE          ///
//////////////////////////////

namespace jam::gfx
{

/// @brief 렌더러를 초기화합니다.
void Init(HWND _hwnd, Int32 _width, Int32 _height);

/// @brief 렌더러를 종료하고 리소스를 정리합니다.
void Shutdown();

/// @brief 내부 렌더링 리소스들의 메모리 누수를 검사합니다.
bool DetectLeak();

/// @brief 이벤트를 처리합니다.
/// @param _event 처리할 이벤트 객체
void OnEvent(Event& _event);

/// @brief 화면에 렌더링 결과를 출력합니다.
/// @param _bVSync 수직 동기화 사용 여부
void Frame(bool _bVSync);

/// @brief Direct3D 11 디바이스를 반환합니다.
/// @return ID3D11Device 포인터
JAM_NODISCARD ID3D11Device* GetDevice();

/// @brief DXGI 스왑체인을 반환합니다.
/// @return IDXGISwapChain1 포인터
JAM_NODISCARD IDXGISwapChain1* GetSwapChain();

/// @brief Direct3D 11 디바이스 컨텍스트를 반환합니다.
/// @return ID3D11DeviceContext 포인터
JAM_NODISCARD ID3D11DeviceContext* GetDeviceContext();

///////////////////////////////////
///          FACTORIES          ///
///////////////////////////////////

/// @brief 수정 불가능한(Immutable) 버텍스 버퍼를 생성합니다.
/// @param _vertexData 버텍스 데이터
/// @param _vertexLayout 버텍스 레이아웃
/// @return 버텍스 버퍼에 대한 참조
/// @note 일반적으로 사용하는 버텍스 버퍼이며, 성능이 뛰어납니다. 단, 생성 후에는 데이터를 수정할 수 없습니다.
JAM_NODISCARD VertexBufferHandle CreateVertexBuffer(
    MemoryView          _vertexData,
    const VertexLayout& _vertexLayout);

/// @brief 수정 가능한(Dynamic) 버텍스 버퍼를 생성합니다.
/// @param _vertexCount 버텍스 개수
/// @param _vertexLayout 버텍스 레이아웃
/// @return 다이나믹 버텍스 버퍼에 대한 참조
/// @note 자주 변경되는 버텍스 데이터를 저장하는데 사용합니다. 성능은 immutable 버텍스 버퍼보다 떨어집니다.
JAM_NODISCARD VertexBufferHandle CreateDynamicVertexBuffer(
    Int32               _vertexCount,
    const VertexLayout& _vertexLayout);

/// @brief 수정 불가능한(Immutable) 인덱스 버퍼를 생성합니다.
/// @param _indexData 인덱스 데이터
/// @param _format 16비트 또는 32비트 인덱스 포맷
/// @return 인덱스 버퍼에 대한 참조
/// @note 일반적으로 사용하는 인덱스 버퍼이며, 성능이 뛰어납니다. 단, 생성 후에는 데이터를 수정할 수 없습니다.
JAM_NODISCARD IndexBufferHandle CreateIndexBuffer(
    MemoryView   _indexData,
    eIndexFormat _format = eIndexFormat::U16);

/// @brief 수정 가능한(Dynamic) 인덱스 버퍼를 생성합니다.
/// @param _indexCount 인덱스 개수
/// @param _format 16비트 또는 32비트 인덱스 포맷
/// @return 다이나믹 인덱스 버퍼에 대한 참조
/// @note 자주 변경되는 인덱스 데이터를 저장하는데 사용합니다. 성능은 immutable 인덱스 버퍼보다 떨어집니다.
JAM_NODISCARD IndexBufferHandle CreateDynamicIndexBuffer(
    Int32        _indexCount,
    eIndexFormat _format = eIndexFormat::U16);

/// @brief 상수 버퍼를 생성합니다.
/// @param _byteWidth 버퍼 크기 (바이트 단위, 16의 배수여야 함)
/// @return 상수 버퍼에 대한 참조
/// @note 상수 버퍼는 다이나믹 버퍼로 생성되며, CPU에서 매 프레임마다 데이터를 업데이트할 수 있습니다.
JAM_NODISCARD ConstantBufferHandle CreateConstantBuffer(
    Int32 _byteWidth);

/// @brief 2D 텍스처를 생성합니다.
/// @param _width 텍스처 너비
/// @param _height 텍스처 높이
/// @param _format 텍스처 포맷
/// @param _arraySize 텍스처 배열 크기
/// @param _flags 텍스처 생성 플래그
/// @param _initData 초기화 데이터 (없으면 빈 배열 전달)
/// @return 2D 텍스처에 대한 참조
/// @note _initData가 제공되면 immutable 텍스처를 생성하고, 아니면 default 텍스처를 생성합니다.
/// 텍스처를 프레임 버퍼로 사용할 경우 _viewFlags에 eViewFlags_RenderTarget 또는 eViewFlags_DepthStencil 플래그를 포함해야 합니다.
JAM_NODISCARD TextureHandle CreateTexture2D(
    Int32                              _width,
    Int32                              _height,
    eFormat                            _format,
    Int32                              _arraySize = 1,
    eTextureCreateFlags                _flags     = eTextureCreateFlags_None,
    const std::span<const MemoryView>& _initData  = {});

/// @brief 큐브맵 텍스처를 생성합니다.
/// @param _width 텍스처 너비
/// @param _height 텍스처 높이
/// @param _format 텍스처 포맷
/// @param _cubeCount 큐브맵 개수
/// @param _flags 텍스처 생성 플래그
/// @param _initData 초기화 데이터 (없으면 빈 배열 전달)
/// @return 큐브맵 텍스처에 대한 참조
/// @note 초기화 데이터가 제공되면 immutable 텍스처로 생성하고, 아니면 default 텍스처를 생성합니다.
JAM_NODISCARD TextureHandle CreateTextureCube(
    Int32                              _width,
    Int32                              _height,
    eFormat                            _format,
    Int32                              _cubeCount = 1,
    eTextureCreateFlags                _flags     = eTextureCreateFlags_None,
    const std::span<const MemoryView>& _initData  = {});

/// @brief 3D 텍스처를 생성합니다.
/// @param _width 텍스처 너비
/// @param _height 텍스처 높이
/// @param _depth 텍스처 깊이
/// @param _format 텍스처 포맷
/// @param _texFlags 텍스처 생성 플래그
/// @param _initData 초기화 데이터 (없으면 빈 배열 전달)
/// @return 3D 텍스처에 대한 참조
/// @note 초기화 데이터가 제공되면 immutable 텍스처로 생성하고, 아니면 default 텍스처를 생성합니다.
JAM_NODISCARD TextureHandle CreateTexture3D(
    Int32                              _width,
    Int32                              _height,
    Int32                              _depth,
    eFormat                            _format,
    eTextureCreateFlags                _texFlags = eTextureCreateFlags_None,
    const std::span<const MemoryView>& _initData = {});

/// @brief 프레임 버퍼를 생성합니다.
/// @param _handles 프레임 버퍼에 첨부할 텍스처 핸들 배열
/// @param _bDestroyTexture true로 설정하면 프레임 버퍼가 파괴될 때 텍스처도 함께 파괴됩니다.
/// 텍스처를 여러 용도로 사용한다면 반드시 false로 설정해야 합니다.
/// @return 프레임 버퍼에 대한 참조
/// @note 렌더 타겟 텍스처는 최대 k_renderTargetCountMax개까지 첨부할 수 있습니다.
/// @note 뎁스 스텐실 텍스처는 최대 1개까지 첨부할 수 있습니다.
JAM_NODISCARD FrameBufferHandle CreateFrameBuffer(
    std::span<const TextureHandle> _handles,
    bool                           _bDestroyTexture = false);

/// @brief 프레임 버퍼를 생성합니다. (simple)
/// @param _handle 프레임 버퍼에 첨부할 단일 텍스처 핸들
/// @param _bDestroyTexture true로 설정하면 프레임 버퍼가 파괴될 때 텍스처도 함께 파괴됩니다.
/// @return 프레임 버퍼에 대한 참조
JAM_NODISCARD FrameBufferHandle CreateFrameBuffer(
    TextureHandle _handle,
    bool          _bDestroyTexture = false);

/// @brief 프레임 버퍼를 생성합니다. (simple)
/// @param _width 프레임 버퍼의 너비
/// @param _height 프레임 버퍼의 높이
/// @param _renderTargetFormat 렌더 타겟 텍스처 포맷
/// @param _depthFormat 뎁스 스텐실 텍스처 포맷
/// @param _writeOnlyRenderTarget 렌더 타겟을 쓰기 전용으로 생성할지 여부
/// @param _writeOnlyDepthStencil 뎁스 스텐실을 쓰기 전용으로 생성할지 여부
/// @return 프레임 버퍼에 대한 참조
JAM_NODISCARD FrameBufferHandle CreateFrameBuffer(
    Int32   _width,
    Int32   _height,
    eFormat _renderTargetFormat,
    eFormat _depthFormat,
    bool    _writeOnlyRenderTarget = false,
    bool    _writeOnlyDepthStencil = false);

/// @brief 프레임 버퍼를 생성합니다.
/// @param _width 프레임 버퍼의 너비
/// @param _height 프레임 버퍼의 높이
/// @param _format 렌더 타겟 텍스처 포맷
/// @param _bWriteOnly 쓰기 전용으로 생성할지 여부
/// @return 프레임 버퍼에 대한 참조
JAM_NODISCARD FrameBufferHandle CreateFrameBuffer(
    Int32   _width,
    Int32   _height,
    eFormat _format,
    bool    _bWriteOnly = false);

/// @brief 쉐이더를 생성합니다.
/// @param _shaderByteCode 쉐이더 바이트코드
/// @return 셰이더에 대한 참조
JAM_NODISCARD ShaderHandle CreateShader(
    MemoryView _shaderByteCode);

/// @brief 프로그램을 생성합니다.
/// @param _vsHandle 버텍스 셰이더 핸들
/// @param _psHandle 픽셀 셰이더 핸들
/// @param _gsHandle 지오메트리 셰이더 핸들 (k_invalidHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
/// @param _dsHandle 도메인 셰이더 핸들 (k_invalidHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
/// @param _hsHandle 홀 셰이더 핸들 (k_invalidHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
/// @param _bDestroyShader true로 설정하면 프로그램이 파괴될 때 셰이더도 함께 파괴됩니다.
/// @return 프로그램에 대한 참조
/// @note nullopt가 전달되면 해당 셰이더 스테이지는 생성되지 않습니다.
JAM_NODISCARD ProgramHandle CreateProgram(
    ShaderHandle _vsHandle,
    ShaderHandle _psHandle,
    ShaderHandle _gsHandle,
    ShaderHandle _dsHandle,
    ShaderHandle _hsHandle,
    bool         _bDestroyShader = false);

/// @brief 프로그램을 생성합니다. (버텍스 셰이더 + 픽셀 셰이더 버전)
/// @param _vsHandle 버텍스 셰이더 핸들
/// @param _psHandle 픽셀 셰이더 핸들
/// @param _bDestroyShader true로 설정하면 프로그램이 파괴될 때 셰이더도 함께 파괴됩니다.
/// @return 프로그램에 대한 참조
JAM_NODISCARD ProgramHandle CreateProgram(
    ShaderHandle _vsHandle,
    ShaderHandle _psHandle,
    bool         _bDestroyShader = false);

////////////////////////////////////
///          DESTRUCTOR          ///
////////////////////////////////////

void DestroyVertexBuffer(VertexBufferHandle _handle);
void DestroyIndexBuffer(IndexBufferHandle _handle);
void DestroyTexture(TextureHandle _handle);
void DestroyFrameBuffer(FrameBufferHandle _handle);
void DestroyProgram(ProgramHandle _handle);
void DestroyShader(ShaderHandle _handle);
void DestroyConstantBuffer(ConstantBufferHandle _handle);

//////////////////////////////////////////
///          PIPELINE BINDING          ///
//////////////////////////////////////////

/// @brief 버텍스 버퍼에 디버깅용 이름을 설정합니다.
/// @param _handle 버텍스 버퍼 리소스
/// @param _name 버텍스 버퍼 이름
/// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
void SetVertexBufferName(
    VertexBufferHandle _handle,
    std::string_view   _name = "");

/// @brief 인덱스 버퍼에 디버깅용 이름을 설정합니다.
/// @param _handle 인덱스 버퍼 리소스
/// @param _name 인덱스 버퍼 이름
/// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
void SetIndexBufferName(
    IndexBufferHandle _handle,
    std::string_view  _name = "");

/// @brief 상수 버퍼에 디버깅용 이름을 설정합니다.
/// @param _handle 상수 버퍼 리소스
/// @param _name 상수 버퍼 이름
/// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
void SetConstantBufferName(
    ConstantBufferHandle _handle,
    std::string_view     _name = "");

/// @brief 텍스처에 디버깅용 이름을 설정합니다.
/// @param _handle 텍스처 리소스
/// @param _name 텍스처 이름
/// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
void SetTextureName(
    TextureHandle    _handle,
    std::string_view _name = "");

/// @brief 프레임 버퍼에 디버깅용 이름을 설정합니다.
/// @param _handle 프레임 버퍼 리소스
/// @param _name 프레임 버퍼 이름
/// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
void SetFrameBufferName(
    FrameBufferHandle _handle,
    std::string_view  _name = "");

/// @brief 프로그램에 디버깅용 이름을 설정합니다.
/// @param _handle 프로그램 리소스
/// @param _name 프로그램 이름
void SetShaderName(
    ShaderHandle     _handle,
    std::string_view _name = "");

///////////////////////////////////////////
///          TEXTURE UTILITIES          ///
///////////////////////////////////////////

/// @brief 텍스처를 로드합니다.
/// @param _scratchImage 스크래치 이미지
/// @return 텍스처 핸들
/// ImageIO 클래스를 통해 이미지 파일을 로드 및 전처리한 후, 이 함수를 사용하여 텍스처를 생성할 수 있습니다.
JAM_NODISCARD TextureHandle LoadTexture(
    const DirectX::ScratchImage& _scratchImage);

/// @brief 텍스처를 로드합니다.
/// @param _filePath 이미지 파일 경로
/// @param _bSRGB sRGB 텍스처로 로드할지 여부
/// @param _bGenMips MIP 맵을 생성할지 여부
/// @return 텍스처 핸들
/// 경로만으로 텍스처를 로드하는 헬퍼 함수입니다. 내부적으로 ImageIO 클래스를 사용합니다.
JAM_NODISCARD Result<TextureHandle> LoadTexture(
    const fs::path& _filePath,
    bool            _bSRGB    = false,
    bool            _bGenMips = false);

/// @brief 2D 텍스처에 데이터를 업로드합니다.
/// @param _handle 텍스처 핸들
/// @param _uploadTexData 업로드할 텍스처 데이터
/// @param _uploadTexWidth 업로드할 텍스처 너비
/// @param _uploadTexHeight 업로드할 텍스처 높이
/// @param _mipIndex 업로드할 MIP 레벨
/// @param _arrayIndex 업로드할 배열 인덱스
/// @param _xOffset 텍스처 내 X 오프셋 (픽셀 단위)
/// @param _yOffset 텍스처 내 Y 오프셋 (픽셀 단위)
/// @note Immutable 텍스처에는 데이터를 업로드할 수 없습니다.
void UploadTexture2D(
    TextureHandle _handle,
    MemoryView    _uploadTexData,
    Int32         _uploadTexWidth,
    Int32         _uploadTexHeight,
    Int32         _mipIndex   = 0,
    Int32         _arrayIndex = 0,
    Int32         _xOffset    = 0,
    Int32         _yOffset    = 0);

/// @brief 큐브맵 텍스처에 데이터를 업로드합니다.
/// @param _handle 텍스처 핸들
/// @param _uploateTexData 업로드할 텍스처 데이터
/// @param _uploadTexWidth 업로드할 텍스처 너비
/// @param _uploadTexHeight 업로드할 텍스처 높이
/// @param _mipIndex 업로드할 MIP 레벨
/// @param _cubeIndex 업로드할 큐브 인덱스 (0~5)
/// @param _cubeFace 업로드할 큐브 페이스
/// @param _xOffset 텍스처 내 X 오프셋 (픽셀 단위)
/// @param _yOffset 텍스처 내 Y 오프셋 (픽셀 단위)
/// @note Immutable 텍스처에는 데이터를 업로드할 수 없습니다.
void UploadTextureCube(
    TextureHandle _handle,
    MemoryView    _uploateTexData,
    Int32         _uploadTexWidth,
    Int32         _uploadTexHeight,
    Int32         _mipIndex  = 0,
    Int32         _cubeIndex = 0,
    eCubeFace     _cubeFace  = eCubeFace::Right,
    Int32         _xOffset   = 0,
    Int32         _yOffset   = 0);

/// @brief 3D 텍스처에 데이터를 업로드합니다.
/// @param _handle 텍스처 핸들
/// @param _uplodeTexData 업로드할 텍스처 데이터
/// @param _uploadTexWidth 업로드할 텍스처 너비
/// @param _uploadTexHeight 업로드할 텍스처 높이
/// @param _uploadTexDepth 업로드할 텍스처 깊이
/// @param _mipIndex 업로드할 MIP 레벨
/// @param _xOffset 텍스처 내 X 오프셋 (픽셀 단위)
/// @param _yOffset 텍스처 내 Y 오프셋 (픽셀 단위)
/// @param _zOffset 텍스처 내 Z 오프셋 (픽셀 단위)
/// @note Immutable 텍스처에는 데이터를 업로드할 수 없습니다.
void UploadTexture3D(
    TextureHandle _handle,
    MemoryView    _uplodeTexData,
    Int32         _uploadTexWidth,
    Int32         _uploadTexHeight,
    Int32         _uploadTexDepth,
    Int32         _mipIndex = 0,
    Int32         _xOffset  = 0,
    Int32         _yOffset  = 0,
    Int32         _zOffset  = 0);

/// @brief 2D 텍스처에서 데이터를 다운로드합니다.
/// @param _handle 텍스처 핸들
/// @param _mipIndex 다운로드할 MIP 레벨
/// @param _arrayIndex 다운로드할 배열 인덱스
/// @return 다운로드된 텍스처 데이터
/// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadTexture2D(
    TextureHandle _handle,
    Int32         _mipIndex   = 0,
    Int32         _arrayIndex = 0);

/// @brief 큐브맵 텍스처에서 데이터를 다운로드합니다.
/// @param _handle 텍스처 핸들
/// @param _mipIndex 다운로드할 MIP 레벨
/// @param _cubeIndex 다운로드할 큐브 인덱스 (0~5)
/// @param _cubeFace 다운로드할 큐브 페이스
/// @return 다운로드된 텍스처 데이터
/// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadTextureCube(
    TextureHandle _handle,
    Int32         _mipIndex  = 0,
    Int32         _cubeIndex = 0,
    eCubeFace     _cubeFace  = eCubeFace::Right);

/// @brief 3D 텍스처에서 데이터를 다운로드합니다.
/// @param _handle 텍스처 핸들
/// @param _mipIndex 다운로드할 MIP 레벨
/// @return 다운로드된 텍스처 데이터
/// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadTexture3D(
    TextureHandle _handle,
    Int32         _mipIndex = 0);

/// @brief 텍스처를 DirectX::ScratchImage로 캡처합니다.
/// @param _handle 텍스처 핸들
/// @return 캡처된 스크래치 이미지
JAM_NODISCARD DirectX::ScratchImage Capture(
    TextureHandle _handle);

/// @brief 멀티샘플링된 텍스처를 멀티샘플링되지 않은 텍스처로 해상합니다.
/// @param _srcHandle MSAA 텍스처 핸들
/// @param _srcArrayIndex MSAA 텍스처 배열 인덱스
/// @param _dstHandle 해상할 텍스처 핸들
/// @param _dstArrayIndex 해상할 텍스처 배열 인덱스
/// @note mipmap 레벨은 0으로 고정됩니다.
JAM_NODISCARD void Resolve(
    TextureHandle _srcHandle,
    Int32         _srcArrayIndex,
    TextureHandle _dstHandle,
    Int32         _dstArrayIndex);

/// @brief 텍스처 간 데이터를 복사합니다.
/// @param _srcHandle 복사할 원본 텍스처 핸들
/// @param _dstHandle 복사할 대상 텍스처 핸들
/// @param _blitWidthOrZero 복사할 너비 (0이면 전체 너비)
/// @param _blitHeightOrZero 복사할 높이 (0이면 전체 높이)
/// @param _blitDepthOrZero 복사할 깊이 (0이면 전체 깊이, 3D 텍스처에서만 사용)
/// @param _srcMipIndex 복사할 원본 MIP 레벨
/// @param _srcArrayIndex 복사할 원본 배열 인덱스
/// @param _srcX 복사할 원본 X 오프셋
/// @param _srcY 복사할 원본 Y 오프셋
/// @param _srcZ 복사할 원본 Z 오프셋 (3D 텍스처에서만 사용)
/// @param _dstMipIndex 복사할 대상 MIP 레벨
/// @param _dstArrayIndex 복사할 대상 배열 인덱스
/// @param _dstX 복사할 대상 X 오프셋
/// @param _dstY 복사할 대상 Y 오프셋
/// @param _dstZ 복사할 대상 Z 오프셋 (3D 텍스처에서만 사용)
void Blit(
    TextureHandle _srcHandle,
    TextureHandle _dstHandle,
    Int32         _blitWidthOrZero  = 0,
    Int32         _blitHeightOrZero = 0,
    Int32         _blitDepthOrZero  = 0,
    Int32         _srcMipIndex      = 0,
    Int32         _srcArrayIndex    = 0,
    Int32         _srcX             = 0,
    Int32         _srcY             = 0,
    Int32         _srcZ             = 0,
    Int32         _dstMipIndex      = 0,
    Int32         _dstArrayIndex    = 0,
    Int32         _dstX             = 0,
    Int32         _dstY             = 0,
    Int32         _dstZ             = 0);

/// @brief 텍스처를 백 버퍼로 복사합니다.
/// @param _srcHandle 복사할 원본 텍스처 핸들
/// @param _blitWidthOrZero 복사할 너비 (0이면 전체 너비)
/// @param _blitHeightOrZero 복사할 높이 (0이면 전체 높이)
/// @param _srcMipIndex 복사할 원본 MIP 레벨
/// @param _srcArrayIndex 복사할 원본 배열 인덱스
/// @param _srcX 복사할 원본 X 오프셋
/// @param _srcY 복사할 원본 Y 오프셋
/// @param _dstX 복사할 대상 X 오프셋
/// @param _dstY 복사할 대상 Y 오프셋
void BlitToBackBuffer(
    TextureHandle _srcHandle,
    Int32         _blitWidthOrZero  = 0,
    Int32         _blitHeightOrZero = 0,
    Int32         _srcMipIndex      = 0,
    Int32         _srcArrayIndex    = 0,
    Int32         _srcX             = 0,
    Int32         _srcY             = 0,
    Int32         _dstX             = 0,
    Int32         _dstY             = 0);

////////////////////////////////////////////////
///          FRAME BUFFER UTILITIES          ///
////////////////////////////////////////////////

/// @brief 렌더 타겟을 특정 색상으로 클리어합니다.
/// @param _handle 프레임 버퍼 핸들 (k_invalidResource 사용 시 기본 프레임 버퍼)
/// @param _color 클리어 색상
/// @param _renderTargetIndex 클리어할 렌더 타겟 인덱스
void ClearRenderTarget(
    FrameBufferHandle _handle,
    RGBA              _color,
    Int32             _renderTargetIndex = 0);

/// @brief 깊이 버퍼와 스텐실 버퍼를 클리어합니다.
/// @param _handle 프레임 버퍼 핸들 (k_invalidResource 사용 시 기본 프레임 버퍼)
/// @param _bClearDepth 깊이 클리어 여부
/// @param _bClearStencil 스텐실 클리어 여부
/// @param _depth 클리어할 깊이 값 (0.0 ~ 1.0)
/// @param _stencil 클리어할 스텐실 값
void ClearDepthStencil(
    FrameBufferHandle _handle,
    bool              _bClearDepth,
    bool              _bClearStencil,
    float             _depth   = 1.f,
    UInt8             _stencil = 0);

/// @brief 프레임 버퍼의 렌더 타겟 텍스처를 반환합니다.
/// @param _handle 프레임 버퍼 핸들
/// @param _renderTargetIndex 렌더 타겟 인덱스
/// @note k_invalidResource를 전달하면 기본 프레임 버퍼의 렌더 타겟 텍스처를 반환합니다.
/// render target texture가 없으면 assertion 처리합니다.
/// 해당 함수로 반환된 텍스처는 DestroyTexture()로 파괴하지 마십시오.
const TextureHandle& GetRenderTargetTexture(
    FrameBufferHandle _handle,
    Int32             _renderTargetIndex = 0);

/// @brief 프레임 버퍼의 깊이-스텐실 텍스처를 반환합니다.
/// @param _handle 프레임 버퍼 핸들
/// @note k_invalidResource를 전달하면 기본 프레임 버퍼의 깊이-스텐실 텍스처를 반환합니다.
/// depth stencil texture가 없으면 assertion 처리합니다.
/// 해당 함수로 반환된 텍스처는 DestroyTexture()로 파괴하지 마십시오.
const TextureHandle& GetDepthStencilTexture(
    FrameBufferHandle _handle);

/// @brief 바인딩된 프레임 버퍼를 즉시 제출합니다.
/// @note 일반적으로 프레임 버퍼는 드로우 콜이 호출되어야 제출하지만.
/// 해당 함수를 호출하면 즉시 제출됩니다.
void Touch();

/////////////////////////////////////////
///          BUFFER UTILITES          ///
/////////////////////////////////////////

/// @brief 버텍스 버퍼에 데이터를 업로드합니다.
/// @param _handle 업로드할 버텍스 버퍼 핸들
/// @param _writeData 업로드할 버텍스 데이터
/// @param _offset 버텍스 버퍼 내의 오프셋 (바이트 단위)
/// @param _bDiscard true: 기존 데이터를 모두 초기화 (빠른 업로드), false: 기존 데이터를 유지하면서 부분 업로드 (느린 업로드)
/// @note Dynamic VertexBuffer에만 데이터를 업로드할 수 있습니다.
void UploadBuffer(
    VertexBufferHandle _handle,
    MemoryView         _writeData,
    Int32              _offset   = 0,
    bool               _bDiscard = true);

/// @brief 인덱스 버퍼에 데이터를 업로드합니다.
/// @param _handle 업로드할 인덱스 버퍼 핸들
/// @param _writeData 업로드할 인덱스 데이터
/// @param _offset 버퍼 내의 오프셋 (바이트 단위)
/// @param _bDiscard true: 기존 데이터를 모두 초기화 (빠른 업로드), false: 기존 데이터를 유지하면서 부분 업로드 (느린 업로드)
/// @note Dynamic IndexBuffer에만 데이터를 업로드할 수 있습니다.
void UploadBuffer(
    IndexBufferHandle _handle,
    MemoryView        _writeData,
    Int32             _offset   = 0,
    bool              _bDiscard = true);

/// @brief 상수 버퍼에 데이터를 업로드합니다.
/// @param _handle 업로드할 상수 버퍼 핸들
/// @param _writeData 업로드할 상수 버퍼 데이터
/// @note 상수 버퍼는 부분 업로드가 불가능하며, 항상 버퍼 전체를 덮어씁니다.
void UploadBuffer(
    ConstantBufferHandle _handle,
    MemoryView           _writeData);

/// @brief 버텍스 버퍼에서 데이터를 다운로드합니다.
/// @param _handle 다운로드할 버텍스 버퍼 핸들
/// @return 다운로드된 버퍼 데이터
/// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadBuffer(
    VertexBufferHandle _handle);

/// @brief 인덱스 버퍼에서 데이터를 다운로드합니다.
/// @param _handle 다운로드할 인덱스 버퍼 핸들
/// @return 다운로드된 버퍼 데이터
/// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadBuffer(
    IndexBufferHandle _handle);

/// @brief 상수 버퍼에서 데이터를 다운로드합니다.
/// @param _handle 다운로드할 상수 버퍼 핸들
/// @return 다운로드된 버퍼 데이터
/// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
JAM_NODISCARD std::vector<Byte> DownloadBuffer(
    ConstantBufferHandle _handle);

/// @brief 버텍스 버퍼의 버텍스 레이아웃을 반환합니다.
/// @param _handle 버텍스 버퍼 핸들
/// @return 버텍스 레이아웃
JAM_NODISCARD const VertexLayout& GetVertexLayout(
    VertexBufferHandle _handle);

///////////////////////////////////////////////
///          PIPELINE STATE BINDER          ///
///////////////////////////////////////////////

/// @brief 버텍스 버퍼를 파이프라인에 바인딩합니다.
/// @param _handleOrInvalid 버텍스 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
void SetVertexBuffer(
    VertexBufferHandle _handleOrInvalid);

/// @brief 버텍스 버퍼를 오프셋과 개수를 지정하여 파이프라인에 바인딩합니다.
/// @param _handleOrInvalid 버텍스 버퍼 핸들
/// @param _vertexCount 버텍스 개수
/// @param _vertexOffset 버퍼 내의 오프셋 (버텍스 단위)
void SetVertexBuffer(
    VertexBufferHandle _handleOrInvalid,
    Int32              _vertexCount,
    Int32              _vertexOffset);

/// @brief 인덱스 버퍼를 파이프라인에 바인딩합니다.
/// @param _handleOrInvalid 인덱스 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
void SetIndexBuffer(
    IndexBufferHandle _handleOrInvalid);

/// @brief 인덱스 버퍼를 오프셋과 개수를 지정하여 파이프라인에 바인딩합니다.
/// @param _handleOrInvalid 인덱스 버퍼 핸들
/// @param _indexCount 인덱스 개수
/// @param _indexOffset 버퍼 내의 오프셋 (인덱스 단위)
void SetIndexBuffer(
    IndexBufferHandle _handleOrInvalid,
    Int32             _indexCount,
    Int32             _indexOffset);

/// @brief 상수 버퍼를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
/// @param _handleOrInvalid 바인딩할 상수 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
/// @param _shader 바인딩할 셰이더 스테이지
/// @param _slot 바인딩 슬롯
void SetConstantBuffer(
    ConstantBufferHandle _handleOrInvalid,
    eShader              _shader,
    Int32                _slot);

/// @brief 텍스처를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
/// @param _handleOrInvalid 바인딩할 텍스처 핸들 (k_invalidResource 사용 시 바인딩 해제)
/// @param _shader 바인딩할 셰이더 스테이지
/// @param _slot 바인딩 슬롯
void SetTexture(
    TextureHandle _handleOrInvalid,
    eShader       _shader,
    Int32         _slot);

/// @brief 샘플러 상태를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
/// @param _state 샘플러 상태 플래그
/// @param _shader 바인딩할 셰이더 스테이지
/// @param _slot 바인딩 슬롯
void SetSampler(
    SamplerState _state,
    eShader      _shader,
    Int32        _slot);

/// @brief Border 샘프러 상태의 테두리 색상을 설정합니다.
/// @param _shader 셰이더 스테이지
/// @param _slot 텍스처 슬롯 번호 (0 ~ 15)
/// @param _rgba 테두리 색상
void SetBorderColor(
    eShader _shader,
    Int32   _slot,
    RGBA    _rgba);

/// @brief 렌더링 상태를 설정합니다.
/// @param _state 렌더링 상태 플래그
void SetGfxState(
    GfxState _state);

/// @brief 블렌드 상태를 설정합니다.
/// @param _renderTargetIndex 렌더 타겟 인덱스 (0 ~ 7)
/// @param _state 블렌드 상태 플래그
void SetBlendState(
    Int32      _renderTargetIndex,
    BlendState _state);

/// @brief 블렌드 팩터를 설정합니다.
/// @param _rgba 블렌딩 시 사용할 색상 팩터
void SetBlendFactor(
    RGBA _rgba);

/// @brief 스텐실 테스트 설정을 구성합니다.
/// @param _fStencil 전면 스텐실 상태
/// @param _bStencil 후면 스텐실 상태
/// @param _stencilRef 스텐실 참조 값
void SetStencil(
    StencilState _fStencil,
    StencilState _bStencil,
    UInt8        _stencilRef = 0);

/// @brief 프로그램을 파이프라인에 바인딩합니다.
/// @param _handleOrInvalid 프로그램 핸들 (k_invalidResource 사용 시 바인딩 해제)
void SetProgram(
    ProgramHandle _handleOrInvalid);

/// @brief 프레임 버퍼를 렌더 타겟으로 설정합니다.
/// @param _handleOrInvalid 프레임 버퍼 핸들 (k_invalidResource 사용 시 기본 프레임 버퍼)
void SetFrameBuffer(
    FrameBufferHandle _handleOrInvalid);

/// @brief 뷰포트를 설정합니다.
/// @param _viewport 뷰포트 설정
void SetViewport(
    Viewport _viewport);

/// @brief 뷰포트를 개별 매개변수로 설정합니다.
/// @param _x 뷰포트 X 좌표
/// @param _y 뷰포트 Y 좌표
/// @param _width 뷰포트 너비
/// @param _height 뷰포트 높이
void SetViewport(
    UInt16 _x,
    UInt16 _y,
    UInt16 _width,
    UInt16 _height);

/// @brief 프레임 버퍼에 대한 바인딩 명령을 즉시 실행합니다.
/// @note 일반적으로 바인딩 명령은 Submit 호출 시점에 실행되지만,
/// 이 함수를 호출하면 드로우 콜 없이 프레임 버퍼 바인딩만 즉시 적용할 수 있습니다.
void Touch();

//////////////////////////////////////////
///          SHADER UTILITIES          ///
//////////////////////////////////////////

/// @brief 프로그램에서 특정 셰이더 스테이지의 셰이더를 반환합니다.
/// @param _programHandle 프로그램 핸들
/// @param _shaderType 셰이더 스테이지
/// @return 셰이더 핸들 (해당 스테이지에 셰이더가 없으면 assertion 처리)
/// 해당 함수로 얻는 쉐이더는 DestroyShader로 파괴하지 마십시오.
JAM_NODISCARD ShaderHandle GetShader(
    ProgramHandle _programHandle,
    eShader       _shaderType);

////////////////////////////////////
///          DRAW CALLS          ///
////////////////////////////////////

/// @brief 드로우 콜을 제출합니다.
/// @note Submit을 호출하기 이전에 리소스를 바인딩해야 합니다.
void Submit();

}   // namespace jam::gfx

#define JAM_IS_VALID_HANDLE(_handle) (_handle != jam::k_invalidHandle)

/// 핸들 파괴 매크로
#define JAM_DESTROY_GRAPHICS_HANDLE(_handle, _func) \
    JAM_BEGIN_MACRO_BLOCK                           \
    if (JAM_IS_VALID_HANDLE(_handle))               \
    {                                               \
        (_func)((_handle));                         \
        (_handle) = jam::k_invalidHandle;           \
    }                                               \
    JAM_END_MACRO_BLOCK

#define JAM_DESTROY_VERTEX_BUFFER(_handle)   JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyVertexBuffer)
#define JAM_DESTROY_INDEX_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyIndexBuffer)
#define JAM_DESTROY_CONSTANT_BUFFER(_handle) JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyConstantBuffer)
#define JAM_DESTROY_PROGRAM(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyProgram)
#define JAM_DESTROY_TEXTURE(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyTexture)
#define JAM_DESTROY_FRAME_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyFrameBuffer)
#define JAM_DESTROY_SHADER(_handle)          JAM_DESTROY_GRAPHICS_HANDLE(_handle, jam::gfx::DestroyShader)
