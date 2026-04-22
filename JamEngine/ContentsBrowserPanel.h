#pragma once
#include "Event.h"
#include "Textures.h"
#include "ThumbnailLoader.h"

namespace jam
{

class FileModifiedEvent;
class FileRemoveEvent;
class FileAddEvent;
class FileRenamedEvent;
class EditorLayer;
enum class eAssetType;

class ContentsBrowserPanel
{
    struct Content
    {
        fs::path    path;
        std::string displayName;
        std::string displayPath;
        TextureRef   thumbnail;
    };

    struct DirectoryNode
    {
        std::string                displayName;
        fs::path                   path;
        std::vector<DirectoryNode> childDirectories;
    };

public:
    explicit ContentsBrowserPanel(EditorLayer* _pEditorLayer);
    void Show(bool* _pOpenOrNull);
    void OnEvent(Event& _event) const;

private:
    // 렌더링 아이템
    void DrawDirectoryNodeRecursive_(const DirectoryNode& _node);
    void DrawContentInfomationText_(const char* _nameStr, const char* _pathStr, bool _bIsDirectory) const;
    void DrawContentButton_(Content& _contentRef, const ImVec2& _buttonSize);

    // directory 변경 관련 함수
    void SetCurrentDirectory_(const fs::path& _path);
    void SetParentAsCurrentDirectory();

    // 내부적으로 관리하는 파일 시스템
    void                    UpdateFileSystem_();
    void                    UpdateFocusDirectoryContents_();
    JAM_NODISCARD DirectoryNode UpdateDirectoryTreeRecursive_(const fs::path& _path);

    // 렌더링할 컨텐츠 수집
    void CollectFilteredContents_();

    // 이벤트 처리기
    void OnFileAddEvent_(const FileAddEvent& _event);
    void OnFileRemoveEvent_(const FileRemoveEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);

    EventDispatcher m_eventDispatcher;
    EditorLayer*    m_pEditorLayer = nullptr;   // 에디터 레이어 포인터
    TextureRef       m_defaultFileIcon;          // 기본 파일 아이콘 (기본 썸네일로 사용)

    // 파일 시스템
    fs::path             m_currentPath;             // 현재 디렉토리의 경로
    std::string          m_displayCurrentPath;      // 현재 디렉토리의 표시용 문자열
    std::vector<Content> m_currentPathContents;     // 현재 디렉토리의 컨텐츠
    DirectoryNode        m_directoryTree;           // 전체적인 폴더 구조만 저장
    bool                 m_bNeedUpdateFileSystem;   // 파일 시스템 업데이트 필요 여부 (dirty bit 형태로 처리)

    // 렌더링
    std::vector<Content*> m_filteredContentCollection;   // 렌더링할 컨텐츠 컬렉션. (필터링 된 컨텐츠)
    ImGuiTextFilter       m_textFilter;                  // 검색 필터

    // 유틸리티
    ThumbnailLoader m_thumbnailLoader;

    // 컨텐츠 버튼
    constexpr static ImVec2 kContentThumbnailSize = ImVec2 { 128.f, 128.f };   // 컨텐츠 아이콘 크기
    constexpr static ImVec2 kContentButtonSize    = ImVec2 { 100.f, 100.f };   // 컨텐츠 버튼 크기
    float                   m_contentButtonScale   = 1.0f;                      // 컨텐츠 버튼 크기 조절을 위한 스케일
};

}   // namespace jam