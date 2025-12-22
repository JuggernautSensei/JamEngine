#include "pch.h"

#include "ContentsBrowserPanel.h"

#include "Application.h"
#include "BuiltInEditorLayerIcon.h"
#include "EditorLayer.h"
#include "EditorUtilities.h"
#include "IModalBox.h"
#include "ImageBuilder.h"
#include "ModalBoxes.h"
#include "Scene.h"
#include "SceneLayer.h"
#include "WindowsUtilities.h"

using namespace jam;

namespace
{
// 파일 및 디렉토리 이름 변경을 담당하는 헬퍼 함수
void RenameFile(EditorLayer* _pEditorLayer, const fs::path& _oldPath, const fs::path& _newPath)
{
    std::error_code ec;
    fs::rename(_oldPath, _newPath, ec);   // 리네임 시도

    if (ec)   // 실패시
    {
        const std::string errorMessage = std::format("failed to rename file from '{}' to '{}'.\nerror: {}", _oldPath.string(), _newPath.string(), ec.message());
        _pEditorLayer->OpenModalBoxPanel(MakeScope<OkModalBox>("rename file error", errorMessage));   // 에러 메시지 전송
    }
}

// 콘텐츠 이름 변경을 위한 모달 다이얼로그
class RenameContentModalBox : public IModalBoxPanel
{
public:
    RenameContentModalBox(EditorLayer* pEditorLayer, const fs::path& contentPath)
        : m_pEditorLayer(pEditorLayer)
        , m_filePath(contentPath)
        , m_filename(contentPath.filename().string())
    {
        JAM_ASSERT(pEditorLayer, "EditorLayer pointer is null");
    }

    void Show() override
    {
        // 입력 필드 렌더링
        ImGui::Text("rename content:");
        ImGui::InputText("##rename content", &m_filename, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        // 버튼 크기 계산
        const ImVec2 buttonSize = editor::CalcFittedButtonSize(2);

        // OK 버튼 처리
        if (ImGui::Button("ok", buttonSize))
        {
            const fs::path oldPath = m_filePath;
            fs::path       newPath = m_filePath;
            newPath.replace_filename(m_filename);

            if (m_filename.empty())   // 빈 파일 이름일 경우 처리
            {
                m_pEditorLayer->OpenModalBoxPanel(MakeScope<OkModalBox>("rename content error", "content label cannot be empty."));
            }
            else if (fs::exists(newPath))   // 이미 존재할 경우 덮어쓰기 여부 확인
            {
                constexpr const char* const k_title = "rename content error";
                constexpr const char* const k_msg   = "content with the same label already exists.\ndo you want to overwrite it?";

                auto OnClickYesCallback = [this, oldPath, newPath]
                {
                    RenameFile(m_pEditorLayer, oldPath, newPath);
                };

                m_pEditorLayer->OpenModalBoxPanel(MakeScope<YesOrNoModalBox>(k_title, k_msg, OnClickYesCallback));
            }
            else
            {
                RenameFile(m_pEditorLayer, oldPath, newPath);   // 정상적인 플로우
            }

            ImGui::CloseCurrentPopup();
        }

        // Cancel 버튼 처리
        ImGui::SameLine();
        if (ImGui::Button("cancel", buttonSize))
        {
            ImGui::CloseCurrentPopup();
        }
    }

    StringView GetTitle() const override
    {
        return "rename content";
    }

private:
    EditorLayer* m_pEditorLayer;
    fs::path     m_filePath;
    std::string  m_filename;
};

// 콘텐츠 삭제를 위한 모달 다이얼로그
class DeleteContentModalBox : public IModalBoxPanel
{
public:
    DeleteContentModalBox(EditorLayer* editorLayer, const fs::path& path)
        : m_editorLayer(editorLayer)
        , m_path(path)
        , m_displayPath(path.string())
    {
        JAM_ASSERT(editorLayer, "EditorLayer pointer is null");
    }

    void Show() override
    {
        // 삭제 확인 메시지 렌더링
        const char* displayPath = m_displayPath.c_str();
        const bool  isDirectory = fs::is_directory(m_path);

        ImGui::Text("are you sure you want to delete this content?");
        ImGui::Text("path: %s", displayPath);
        ImGui::Text(isDirectory ? "this will delete the entire directory and its contents." : "this will delete the file.");

        const ImVec2 buttonSize = editor::CalcFittedButtonSize(2);

        // Yes 버튼 처리
        if (ImGui::Button("yes", buttonSize))
        {
            std::error_code ec;
            fs::remove_all(m_path, ec);

            if (ec)
            {
                constexpr const char* const k_title      = "delete content error";
                const std::string           errorMessage = std::format("failed to delete content at '{}'.\nerror: {}", m_path.string(), ec.message());

                m_editorLayer->OpenModalBoxPanel(MakeScope<OkModalBox>(k_title, errorMessage));
            }

            ImGui::CloseCurrentPopup();
        }

        // No 버튼 처리
        ImGui::SameLine();
        if (ImGui::Button("no", buttonSize))
        {
            ImGui::CloseCurrentPopup();
        }
    }

    StringView GetTitle() const override
    {
        return "delete content";
    }

private:
    EditorLayer* m_editorLayer;
    fs::path     m_path;
    std::string  m_displayPath;
};

}   // namespace

namespace jam
{
ContentsBrowserPanel::ContentsBrowserPanel(EditorLayer* _pEditorLayer)
    : m_pEditorLayer(_pEditorLayer)
{
    // 기본 파일 아이콘 로드 및 설정
    m_defaultFileIcon.LoadFromMemory(k_fileDDS, k_fileDDSSize, eImageFormat::DDS);
    m_defaultFileIcon.AttachSRV();   // srv 부착

    // 파일 시스템 이벤트 리스너 등록
    m_eventDispatcher.AddListener<FileAddEvent>(JAM_BIND_MEMBER_LISTENER(ContentsBrowserPanel::OnFileAddEvent_));
    m_eventDispatcher.AddListener<FileRemoveEvent>(JAM_BIND_MEMBER_LISTENER(ContentsBrowserPanel::OnFileRemoveEvent_));
    m_eventDispatcher.AddListener<FileModifiedEvent>(JAM_BIND_MEMBER_LISTENER(ContentsBrowserPanel::OnFileModifiedEvent_));
    m_eventDispatcher.AddListener<FileRenamedEvent>(JAM_BIND_MEMBER_LISTENER(ContentsBrowserPanel::OnFileRenamedEvent_));

    // 초기 디렉토리 설정
    SetCurrentDirectory_(Application::GetInstance().GetContentsDirectory());
}

void ContentsBrowserPanel::Show(bool* _pOpenOrNull)
{
    if (ImGui::Begin("contents browser", _pOpenOrNull))
    {
        // 파일 시스템 업데이트 처리 (dirty bit 패턴)
        if (m_bNeedUpdateFileSystem)
        {
            UpdateFileSystem_();   // 파일 시스템 업데이트
            m_bNeedUpdateFileSystem = false;
        }

        // 좌측 패널 - 디렉토리 트리
        ImGui::BeginGroup();
        {
            ImGui::TextUnformatted("directory tree");

            if (ImGui::BeginChild("directory tree", ImVec2 { 200.f, 0.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
            {
                DrawDirectoryNodeRecursive_(m_directoryTree);   // 재귀적으로 디렉토리 트리 렌더링
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();

        // 우측 패널 - 콘텐츠 뷰
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            // 1행: 네비게이션 바
            {
                // 뒤로가기 버튼
                if (ImGui::ArrowButton("#back", ImGuiDir_Left))
                {
                    SetParentAsCurrentDirectory();
                }

                // 메뉴 버튼 todo: 확장
                ImGui::SameLine();
                ImGui::Button("menu");

                // 현재 경로 표시
                ImGui::SameLine();
                ImGui::TextUnformatted("current path:");

                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##currentPath", &m_displayCurrentPath, ImGuiInputTextFlags_ReadOnly);
            }

            // 2행: 필터 및 콘텐츠 뷰
            {
                // 텍스트 필터 컨트롤
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("filter:");
                ImGui::SameLine();

                if (m_textFilter.Draw("##filter", ImGui::GetContentRegionAvail().x * 0.75f))
                {
                    CollectFilteredContents_();   // 재 필터링이 필요함
                }

                // 줌 컨트롤
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat("##zoom", &m_contentButtonScale, 0.01f, 0.1f, 5.0f, "zoom: %.2f", ImGuiSliderFlags_AlwaysClamp);

                // 디버깅용 렌더링 카운터
                Int32 renderedItemCount = 0;   // 디버깅 용 렌더링 된 콘텐츠 수

                // 그리드 레이아웃 계산
                const ImVec2 buttonSize = k_contentButtonSize * m_contentButtonScale;   // 컨텐츠 버튼 크기
                const float  availWidth = ImGui::GetContentRegionAvail().x;

                // 한 행당 버튼 수 및 간격 계산
                const Int32 buttonCountPerRow = static_cast<Int32>(availWidth / buttonSize.x);   // 한 행에 렌더링할 수 있는 버튼 수
                const float buttonSpacingX    = (buttonCountPerRow > 1) ?
                                                    (availWidth - static_cast<float>(buttonCountPerRow) * buttonSize.x) / static_cast<float>(buttonCountPerRow - 1) :
                                                    0.0f;   // 버튼 간격

                // 전체 행 수 계산
                const size_t totalItems = m_filteredContentCollection.size();
                const Int32  totalRows  = (buttonCountPerRow > 0) ?
                                              static_cast<Int32>(totalItems) / buttonCountPerRow + ((totalItems % buttonCountPerRow != 0) ? 1 : 0) :
                                              0;   // 오프셋 포함

                // 콘텐츠 그리드 렌더링
                ImVec2 childWinSize = ImGui::GetContentRegionAvail();
                childWinSize.y -= ImGui::GetTextLineHeightWithSpacing();   // 하단에 디버깅 정보를 출력하기 위해서 공간을 남김

                if (ImGui::BeginChild("contents view", childWinSize, ImGuiChildFlags_Border))
                {
                    // 가상화를 위한 클리핑 처리
                    static ImGuiListClipper clipper;
                    clipper.Begin(totalRows, buttonSize.y + ImGui::GetStyle().ItemSpacing.y);   // 클리핑 시작

                    while (clipper.Step())
                    {
                        // note: 사전 클리핑에 사용한 버튼의 높이보다 렌더링 되는 콘텐츠 버튼은 더 김 (텍스트를 그리기 때문)
                        // 그렇기 때문에 더 많은 콘텐츠가 렌더링 될 수 있음
                        // 그러나 이는 UX에 문제를 주지 않기에 굳이 추가적인 처리를 하지 않음

                        for (int rowIndex = clipper.DisplayStart; rowIndex < clipper.DisplayEnd; ++rowIndex)   // 클리핑된 범위 (단위는 행)
                        {
                            const Int32 startIdx = rowIndex * buttonCountPerRow;                                           // 클리핑된 시작 인덱스
                            const Int32 endIdx   = Min(startIdx + buttonCountPerRow, static_cast<int>(totalItems));   // 클리핑된 끝 인덱스

                            for (Int32 itemIndex = startIdx; itemIndex < endIdx; ++itemIndex)
                            {
                                Content& contentRef = *m_filteredContentCollection[itemIndex];   // 컨텐츠 참조
                                DrawContentButton_(contentRef, buttonSize);                      // 렌더링
                                ++renderedItemCount;                                             // 렌더링된 아이템 수 증가

                                // 행의 마지막 아이템이 아닌 경우 버튼 간격 적용
                                if (itemIndex != endIdx - 1)
                                {
                                    ImGui::SameLine(0.0f, buttonSpacingX);   // 버튼 간격 적용
                                }
                            }
                        }
                    }
                }
                ImGui::EndChild();

                // 디버깅 정보 출력
                ImGui::TextDisabled("count: %d, filtered: %d, rendered: %d",
                                    static_cast<int>(m_currentPathContents.size()),
                                    static_cast<int>(m_filteredContentCollection.size()),
                                    renderedItemCount);
            }
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

void ContentsBrowserPanel::OnEvent(Event& _event) const
{
    m_eventDispatcher.Dispatch(_event);
}

void ContentsBrowserPanel::DrawContentInfomationText_(const char* _nameStr, const char* _pathStr, const bool _bIsDirectory) const
{
    ImGui::Text("label: %s", _nameStr);
    ImGui::Text("asset: %s", _pathStr);
    ImGui::Text("file type: %s", _bIsDirectory ? "directory" : "regular");
}

void ContentsBrowserPanel::DrawContentButton_(Content& _contentRef, const ImVec2& _buttonSize)
{
    // 기본 정보 준비
    const char* displayName  = _contentRef.displayName.c_str();
    const char* displayPath  = _contentRef.displayPath.c_str();
    const bool  bIsDirectory = fs::is_directory(_contentRef.path);

    // 렌더링을 위한 아이템 정보 설정
    const ImVec2         itemLeftTop = ImGui::GetCursorScreenPos();
    ImGuiID              itemID;
    ImRect               itemRect;
    ImGuiItemFlags       itemFlags;
    ImGuiItemStatusFlags itemStatusFlags;

    // 버튼 + 이미지 + 텍스트 렌더링
    {
        ImGui::BeginGroup();
        ImGui::PushID(displayName);

        // 메인 버튼 렌더링 및 더블클릭 처리
        ImGui::Button("##contentButton", _buttonSize);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))   // 더블클릭 시
        {
            if (bIsDirectory)   // 디렉토리인 경우에는 포커스 디렉토리로 설정
            {
                SetCurrentDirectory_(_contentRef.path);
            }
            else   // 파일인 경우에는 파일 탐색기에서 열기
            {
                OpenInFileExplorer(_contentRef.path);
            }
        }

        // 드래그 앤 드롭을 위한 아이템 정보 저장
        {
            itemID          = ImGui::GetItemID();
            itemRect        = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
            itemFlags       = ImGui::GetItemFlags();
            itemStatusFlags = ImGui::GetItemStatusFlags();
        }

        // 썸네일 이미지 오버레이 렌더링
        {
            const TextureRef& thumbnail          = _contentRef.thumbnail;
            const auto [width, height]          = thumbnail.GetSize();
            const ImVec2 innerBox               = { static_cast<float>(width), static_cast<float>(height) };
            const auto [imageSize, imageOffset] = editor::CalcFittedCenteredRect(_buttonSize, innerBox);
            const ImVec2 imageStart             = itemLeftTop + imageOffset;
            const ImVec2 imageEnd               = imageStart + imageSize;
            ImGui::GetWindowDrawList()->AddImage(static_cast<void*>(thumbnail.GetSRV()), imageStart, imageEnd);
        }

        // 텍스트 레이블 렌더링
        {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + _buttonSize.x);
            ImGui::TextUnformatted(displayName);
            ImGui::PopTextWrapPos();
        }
        ImGui::PopID();
        ImGui::EndGroup();
    }

    // 툴팁 표시
    if (ImGui::BeginItemTooltip())
    {
        DrawContentInfomationText_(displayName, displayPath, bIsDirectory);
        ImGui::EndTooltip();
    }

    // 우클릭 컨텍스트 메뉴
    if (ImGui::BeginPopupContextItem(displayName))
    {
        ImGui::Text(displayName);
        ImGui::Separator();

        // 파일 탐색기에서 열기
        if (ImGui::Selectable("open in explorer"))
        {
            const fs::path pathToOpen = bIsDirectory ? _contentRef.path : _contentRef.path.parent_path();
            OpenInFileExplorer(pathToOpen);
        }

        // 에셋 로드 서브메뉴
        if (ImGui::BeginMenu("load asset"))
        {
            bool       bClicked  = false;
            eAssetType assetType = {};

            if (ImGui::MenuItem("as texture"))
            {
                assetType = eAssetType::ImageTexture;
                bClicked  = true;
            }

            if (ImGui::MenuItem("as model"))
            {
                assetType = eAssetType::Model;
                bClicked  = true;
            }

            if (bClicked)
            {
                SceneLayer* pSceneLayer  = Application::GetInstance().GetSceneLayer();
                Scene*      pActiveScene = pSceneLayer->GetActiveScene();
                m_pEditorLayer->LoadAsset(pActiveScene->GetAssetManagerRef(), assetType, _contentRef.path);
            }

            ImGui::EndMenu();
        }

        // 이름 변경
        if (ImGui::Selectable("rename"))
        {
            m_pEditorLayer->OpenModalBoxPanel(MakeScope<RenameContentModalBox>(m_pEditorLayer, _contentRef.path));
        }

        // 삭제
        if (ImGui::Selectable("delete"))
        {
            m_pEditorLayer->OpenModalBoxPanel(MakeScope<DeleteContentModalBox>(m_pEditorLayer, _contentRef.path));
        }

        ImGui::EndPopup();
    }

    // 드래그 앤 드롭을 위한 아이템 상태 복원
    // ImGui::SetLastItemData(itemID, itemFlags, itemStatusFlags, itemRect);

    // 드래그 소스 설정
    if (ImGui::BeginDragDropSource())
    {
        editor::SetDragDropPayload(editor::k_contentPayload, _contentRef.path);
        DrawContentInfomationText_(displayName, displayPath, bIsDirectory);
        ImGui::EndDragDropSource();
    }

    // 디렉토리의 경우 드롭 타겟 설정
    if (bIsDirectory && ImGui::BeginDragDropTarget())
    {
        // 페이로드는 디렉토리 파일 둘 다 허용
        if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(editor::k_contentPayload))
        {
            const fs::path oldPath = editor::GetPathFromPayload(pPayload);
            const fs::path newPath = _contentRef.path / oldPath.filename();
            RenameFile(m_pEditorLayer, oldPath, newPath);
        }
        ImGui::EndDragDropTarget();
    }
}

void ContentsBrowserPanel::SetCurrentDirectory_(const fs::path& _path)
{
    JAM_ASSERT(fs::exists(_path), "Path does not exist: {}", _path.string());
    JAM_ASSERT(fs::is_directory(_path), "Path is not a directory: {}", _path.string());

    if (m_currentPath == _path)   // 현재 디렉토리와 동일하면 아무 작업도 하지 않음
    {
        return;
    }

    m_currentPath           = _path;
    m_displayCurrentPath    = _path.string();   // 현재 디렉토리 표시 업데이트
    m_bNeedUpdateFileSystem = true;             // 파일 시스템 업데이트 필요
}

void ContentsBrowserPanel::SetParentAsCurrentDirectory()
{
    const Application& app = Application::GetInstance();
    if (m_currentPath != app.GetContentsDirectory())   // 컨텐츠 디렉토리보다 이전 디렉토리로는 이동하지 못함
    {
        SetCurrentDirectory_(m_currentPath.parent_path());   // 현재 디렉토리의 부모 디렉토리로 설정
    }
}

void ContentsBrowserPanel::UpdateFileSystem_()
{
    // 루트 디렉토리 트리 업데이트
    m_directoryTree = UpdateDirectoryTreeRecursive_(Application::GetInstance().GetContentsDirectory());

    // 현재 디렉토리의 콘텐츠를 업데이트
    UpdateFocusDirectoryContents_();
}

void ContentsBrowserPanel::UpdateFocusDirectoryContents_()
{
    m_currentPathContents.clear();

    for (const fs::directory_entry& entry: fs::directory_iterator(m_currentPath))
    {
        if (!entry.is_directory() && !entry.is_regular_file())   // 컨텐츠는 디렉토리와 파일만 처리
        {
            continue;
        }

        const fs::path& path = entry.path();

        // 썸네일 로딩 처리
        auto [thumbnail, bResult] = m_thumbnailLoader.CreateThumbnailTexture(path, static_cast<Int32>(k_contentThumbnailSize.x), static_cast<Int32>(k_contentThumbnailSize.y));
        if (bResult)   // 썸네일이 로딩되면 설정
        {
            thumbnail.AttachSRV();   // SRV를 부착
        }
        else   // 로드에 실패하면 기본 아이콘 사용
        {
            thumbnail = m_defaultFileIcon;
        }

        // 컨텐츠 컬렉션에 추가
        Content content {
            .path        = path,
            .displayName = path.filename().string(),
            .displayPath = path.string(),
            .thumbnail   = thumbnail,
        };
        m_currentPathContents.emplace_back(std::move(content));   // 컨텐츠 컬렉션에 추가
    }

    // 필터링된 컨텐츠 컬렉션 업데이트
    CollectFilteredContents_();
}

ContentsBrowserPanel::DirectoryNode ContentsBrowserPanel::UpdateDirectoryTreeRecursive_(const fs::path& _path)
{
    DirectoryNode directoryNode;
    directoryNode.path        = _path;
    directoryNode.displayName = _path.filename().string();

    // 하위 디렉토리 탐색 및 추가
    for (const fs::directory_entry& entry: fs::directory_iterator(_path))   // 디렉토리의 엔트리를 돌면서 디렉토리 노드 업데이트
    {
        if (entry.is_directory())
        {
            const fs::path& childPath = entry.path();
            directoryNode.childDirectories.push_back(UpdateDirectoryTreeRecursive_(childPath));
        }
    }

    return directoryNode;
}

void ContentsBrowserPanel::CollectFilteredContents_()
{
    m_filteredContentCollection.clear();

    for (Content& contentRef: m_currentPathContents)
    {
        // 텍스트 필터 적용 검사
        if (m_textFilter.IsActive() && !m_textFilter.PassFilter(contentRef.displayName.c_str()))
        {
            continue;   // 필터를 통과하지 못하면 건너뜀
        }
        m_filteredContentCollection.emplace_back(&contentRef);   // 필터를 통과하면 컨텐츠를 수집
    }
}

void ContentsBrowserPanel::OnFileAddEvent_(JAM_MAYBE_UNUSED const FileAddEvent& _event)
{
    // dirty bit 형태로 처리하는 이유는
    // 파일 시스템을 업데이트하는 작업은 무거운데
    // 한 프레임 내에서 dirty bit가 여러 번 설정될 수 있기 때문
    // 따라서, 최종적으로 한 번만 업데이트를 수행하도록 함
    m_bNeedUpdateFileSystem = true;
}

void ContentsBrowserPanel::OnFileRemoveEvent_(JAM_MAYBE_UNUSED const FileRemoveEvent& _event)
{
    m_bNeedUpdateFileSystem = true;
}

void ContentsBrowserPanel::OnFileModifiedEvent_(JAM_MAYBE_UNUSED const FileModifiedEvent& _event)
{
    m_bNeedUpdateFileSystem = true;
}

void ContentsBrowserPanel::OnFileRenamedEvent_(JAM_MAYBE_UNUSED const FileRenamedEvent& _event)
{
    m_bNeedUpdateFileSystem = true;
}

void ContentsBrowserPanel::DrawDirectoryNodeRecursive_(const DirectoryNode& _node)
{
    for (const DirectoryNode& childNode: _node.childDirectories)
    {
        ImGui::PushID(childNode.displayName.c_str());   // 디렉토리 이름으로 ID 설정

        // 트리 노드 구조: > 이름
        // '>'를 누르면 트리가 열리면서 하위 디렉토리가 렌더링됨
        // '이름'을 누르면 해당 디렉토리로 이동함
        const bool bTreeNodeOpen = ImGui::TreeNodeEx("##directoryNode", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesToNodes);   // 트리 노드 생성

        // 디렉토리 이름 렌더링 및 클릭 처리
        ImGui::SameLine();
        if (ImGui::Selectable(childNode.displayName.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth))
        {
            SetCurrentDirectory_(childNode.path);   // 클릭 시 해당 디렉토리로 이동
        }

        // 디렉토리 드래그 소스 설정 - 드래그 드롭으로 다른 디렉토리로 전체를 이동시킬 수 있음
        if (ImGui::BeginDragDropSource())
        {
            editor::SetDragDropPayload(editor::k_contentPayload, childNode.path);   // 디렉토리 경로를 드래그 앤 드롭 페이로드로 설정
            ImGui::TextUnformatted(childNode.displayName.c_str());                  // 드래그 앤 드롭 소스에 디렉토리 이름 표시
            ImGui::EndDragDropSource();
        }

        // 디렉토리 드롭 타겟 설정
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(editor::k_contentPayload))   // 페이로드가 있을 경우
            {
                JAM_ASSERT(fs::is_directory(childNode.path), "Directory path is not a directory: {}", childNode.path.string());   // 디렉토리 경로가 디렉토리가 아닐 경우 에러 발생
                const fs::path oldPath = editor::GetPathFromPayload(pPayload);                                                    // 드래그 앤 드롭된 경로 가져오기
                const fs::path newPath = childNode.path / oldPath.filename();                                                     // 새 경로는 현재 디렉토리 + 드래그 앤 드롭된 파일 이름
                RenameFile(m_pEditorLayer, oldPath, newPath);                                                                     // 파일 또는 디렉토리 이동
            }
            ImGui::EndDragDropTarget();
        }

        // 트리 노드가 열려있는 경우 하위 디렉토리 재귀 렌더링
        if (bTreeNodeOpen)   // '>'를 누르면 트리가 열리면서 하위 디렉토리가 렌더링됨
        {
            DrawDirectoryNodeRecursive_(childNode);   // 재귀적으로 렌더링
            ImGui::TreePop();                         // 트리가 열려 있는 경우에는 트리 닫기
        }

        ImGui::PopID();   // 디렉토리 이름으로 설정한 ID 팝
    }
}

}   // namespace jam