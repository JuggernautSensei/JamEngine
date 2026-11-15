# 빌드 복구 및 코드 정리

요구사항은 **구조 파악 → 공통 빌드 오류 수정 → 예제 실행 복구 → 폐기 코드 삭제 → 디렉터리·프로젝트 정리 → Debug/Release 검증** 순서로 처리했다. Visual Studio/MSBuild, C++20, D3D11 구조와 기존 명명·중괄호·소유권 스타일을 유지한다.

## 현재 구성

메인 솔루션은 `JamEngine`, `SandboxApp`, `JamTest`, `JamBenchmark`로 구성된다. Sandbox의 실제 장면과 검증 대상은 `SandboxApp.cpp` 및 `scripts/Test-Examples.ps1`에 등록되어 있다. 후속 작업에서 추가된 장면과 자산도 유지하며 전체 실행 검사에 포함한다.

엔진은 `Application`에서 플랫폼과 하위 시스템을 초기화하고, `SceneManager`가 장면을 실행한다. 장면은 `RenderGraph`로 렌더링 작업을 구성하며 `Graphics`가 D3D11 호출과 자원을 관리한다. 모델·이미지·셰이더 입력은 `ModelImporter`, `Image`, `ShaderBinary`를 사용한다.

## 복구 내용

- v145/x64 Debug·Release 구성을 정리하고 출력·중간 파일 경로를 `Build/`로 통일했다. 런타임에 필요한 Sandbox 자산을 실행 파일 옆으로 복사한다.
- vcpkg의 자동 링크와 저장소에 포함된 ImGui의 중복을 해결하고, 오래된 라이브러리 경로 및 PCH 설정을 수정했다.
- 문자열 경계·해시·경로 처리, UTF-8 파일 입출력, 파일 충돌 정책, 디렉터리 순회, 자원 풀 반복자, 셰이더 바이트코드 로딩을 수정하고 회귀 검사를 추가했다.
- 그래픽 상태 플래그의 비트 추출과 모델·이미지 API의 오래된 호출을 수정했다. Assimp 노드 행렬은 엔진의 행 벡터 관례에 맞게 전치한다.
- 기본 장면 실행, 잘못된 CLI 입력의 종료 코드, 지정 프레임 후 종료, 장면 캡처를 지원한다. Compute의 모든 세부 데모를 자동 실행할 수 있다.
- 전역 `.obj`, `.bin`, `.dds` 제외 규칙 때문에 빠졌던 모델 및 IBL 자산을 복구했다. 복구 원본은 같은 컴퓨터의 `../JamEngine2/JamEngine/SandboxApp/Resources`이며 glTF 버퍼·접근자 정의와 실제 예제 실행으로 일치 여부를 확인했다. 해당 확장자의 Sandbox 원본 자산은 Git에서 제외하지 않는다.
- 단위 테스트는 그래픽 창 없이 실행하고 실제 실패를 프로세스 종료 코드에 반영한다.

## 삭제 및 배치 기준

- 사용하지 않는 `BasicRenderer`, 명시적으로 폐기된 `ExampleApp`, 실험용 `TestApp`, 구 에디터 구현과 구 API 중 활성 프로젝트에서 참조하지 않는 파일을 삭제했다.
- 현재 작업 트리에서 제거된 `JamShader` 도구는 다시 복원하지 않는다. 관련 프로젝트 참조와 전용 잔여 자산도 정리한다.
- `JamEngine/Shaders`와 `JamEngine/Resources`에 남아 있던 구 렌더러·에디터 자산 및 생성 헤더를 제거했다. 활성 예제는 `SandboxApp/Resources`의 자산을 사용한다.
- 구현이 사라진 `CheckOverflow` 전용 테스트를 삭제했다. 기능이 남아 있는 오래된 테스트는 폐기 기능으로 간주하지 않는다.
- 프로젝트·필터·솔루션 항목을 함께 정리한다. 빌드 산출물, 크래시 덤프, 로그, 캐시, IDE 개인 설정을 Git 추적 대상에서 제외한다.

## 검증 범위

빌드와 실행 명령은 [README.md](../README.md)에 있다. 전체 예제 검사는 반드시 `-All`을 사용한다. 장면마다 종료 코드 0, 오류 로그 없음, PNG 생성을 확인하며 피킹 장면은 예상 ID도 검사한다. SSR·피킹·파티클 전용 검사는 WARP에서 실제 셰이더를 실행한다. 이는 짧은 실행 검사이며 모든 GUI 조작이나 장시간 실행까지 보증하지는 않는다.

`JamTest.vcxproj`에 기존부터 제외되어 있던 `BoundingVolumeTest`, `EnumUtilitiesTest`, `FileSystemTest`, `FileWriterTest2`, `JsonArchiveTest`, `MathTest`, `SceneNodeTest`는 이전 API에 맞춰 작성된 이관 대상이다. 프로젝트에 등록되지 않은 `FileWriterTest.cpp`도 이전 파일 API를 사용한다. 이 파일들은 실행된 테스트 수에 포함하지 않는다. 현재 등록된 테스트의 통과와 모든 과거 테스트의 이관 완료를 구분한다.

검증 로그는 `Build/Logs/`, 장면별 로그·캡처는 `Build/Validation/<구성>/`에 남긴다.

2026-11-15 확인 결과: Debug/Release 솔루션 빌드 성공, 각 구성에서 단위 테스트 359개 및 전체 예제 37개 실행 항목 통과. 두 구성의 벤치마크 실행, SSR·피킹·파티클 GPU 회귀 검사, 다른 작업 디렉터리에서의 기본 실행 및 잘못된 CLI 입력의 종료 코드도 확인했다. PBR·스키닝 등의 캡처에서 실제 모델 렌더링을 확인했다.
