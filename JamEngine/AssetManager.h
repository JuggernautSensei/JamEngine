#pragma once
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"

namespace jam
{

template<typename T>
concept IsAssetBased = std::is_base_of_v<IAsset, T>;

namespace asset_manager
{
    /// @brief 현재 할당된 모든 에셋을 로그로 출력합니다.
    /// @note 주로 디버깅 용도로 사용됩니다.
    bool DetectLeak();

    /// @brief Load*** 에셋을 로드하고 핸들을 반환합니다.
    /// @param _path 에셋 경로 (반드시 working directory 내부 경로)
    /// @param _bOverwrite 이미 로드된 에셋이 있을 경우 덮어쓸지 여부
    /// @return 에셋 핸들
    /// @note 로드는 실패할 수 있으며 실패시 에러 코드를 반환합니다.
    /// @note _path가 working directory 내부에 있어야 하는 이유는 추적 및 관리의 용이성 때문입니다.

    Result<MeshHandle>         LoadMesh(const fs::path& _path, bool _bOverwrite = true);
    Result<MaterialHandle>     LoadMaterial(const fs::path& _path, bool _bOverwrite = true);
    Result<ImageTextureHandle> LoadImageTexture(const fs::path& _path, bool _bOverwrite = true);

    /// @brief Unload*** 에셋을 언로드합니다.
    /// @param _handle 에셋 핸들
    /// @note 언로드 후 핸들은 k_invalidHandle이 되지 않습니다. 사용자가 직접 관리해야 합니다.

    void UnloadMesh(MeshHandle _handle);
    void UnloadMaterial(MaterialHandle _handle);
    void UnloadImageTexture(ImageTextureHandle _handle);

    /// @brief Get***Handle 에셋 핸들을 반환합니다.
    /// @param _path 에셋 경로  (반드시 working directory 내부 경로)
    /// @return 에셋 핸들, 로드되지 않았거나 잘못된 경로일 경우 assert가 발생합니다.

    JAM_NODISCARD MeshHandle         GetMeshHandle(const fs::path& _path);
    JAM_NODISCARD MaterialHandle     GetMaterialHandle(const fs::path& _path);
    JAM_NODISCARD ImageTextureHandle GetImageTextureHandle(const fs::path& _path);

    /// @brief GetOrLoad*** 에셋 핸들을 반환합니다. 로드되지 않았을 경우 로드 시도 후 반환합니다.
    /// @param _path 에셋 경로  (반드시 working directory 내부 경로)
    /// @return 에셋 핸들, 로드에 실패할 경우 에러 코드를 반환합니다.

    JAM_NODISCARD Result<MeshHandle> GetOrLoadMesh(const fs::path& _path);
    JAM_NODISCARD Result<MaterialHandle> GetOrLoadMaterial(const fs::path& _path);
    JAM_NODISCARD Result<ImageTextureHandle> GetOrLoadImageTexture(const fs::path& _path);

    /// @brief Is***Loaded 에셋이 로드되었는지 여부를 반환합니다.
    /// @param _path 에셋 경로
    /// @return 로드 여부

    JAM_NODISCARD bool IsMeshLoaded(const fs::path& _path);
    JAM_NODISCARD bool IsMaterialLoaded(const fs::path& _path);
    JAM_NODISCARD bool IsImageTextureLoaded(const fs::path& _path);

    /// @brief Get*** 에셋 객체의 읽기 전용 참조를 반환합니다.
    /// @param _handle 에셋 핸들
    /// @return 에셋 객체의 읽기 전용 참조, 로드되지 않았거나 잘못된 핸들일 경우 assert가 발생합니다.

    JAM_NODISCARD const Mesh&         GetMesh(MeshHandle _handle);
    JAM_NODISCARD const Material&     GetMaterial(MaterialHandle _handle);
    JAM_NODISCARD const ImageTexture& GetImageTexture(ImageTextureHandle _handle);

    /// @brief Save*** 에셋을 지정한 경로에 저장합니다.
    /// @param _handle 에셋 핸들
    /// @param _path 저장할 경로
    /// @return 저장 성공 여부

    eErrorCode SaveMesh(MeshHandle _handle, const fs::path& _path);
    eErrorCode SaveMaterial(MaterialHandle _handle, const fs::path& _path);
    eErrorCode SaveImageTexture(ImageTextureHandle _handle, const fs::path& _path);

    /// @brief 에셋 핸들 컨테이너에 대한 읽기 전용 참조를 반환합니다.
    JAM_NODISCARD const HashMap<fs::path, MeshHandle>& GetAllMeshHandles();
    JAM_NODISCARD const HashMap<fs::path, MaterialHandle>& GetAllMaterialHandles();
    JAM_NODISCARD const HashMap<fs::path, ImageTextureHandle>& GetAllImageTextureHandles();

}   // namespace asset_manager

}   // namespace jam