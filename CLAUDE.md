# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 빌드

Visual Studio 2022 솔루션 기반 (MSBuild). CMake 사용 안 함.

```
JamEngine.sln          # 메인 솔루션
```

**빌드 구성:**
- `Debug|x64`, `Debug_Editor|x64`
- `Release|x64`, `Release_Editor|x64`

**빌드 출력 경로:** `$(ProjectDir)Bin\$(Configuration)\`

## 프로젝트 구성

| 프로젝트 | 역할 |
|----------|------|
| `JamEngine` | 엔진 정적 라이브러리 |
| `ExampleApp` | 사용 예제 | 그러나 이 프로젝트는 폐기 상태
| `SandboxApp` | 샌드박스 |
| `JamTest` | Google Test 기반 테스트 |
| `JamShader` | HLSL 셰이더 컴파일 도구 |
| `JamBenchmark` | 성능 벤치마크 |

## 테스트 실행

`JamTest` 프로젝트를 빌드 후 실행. `Application`을 상속하며 `Update()`에서 `RUN_ALL_TESTS()`를 호출한다.

## 아키텍처 개요

### Graphics (D3D11 백엔드)
- [GraphicsBase.h](JamEngine/GraphicsBase.h) — 핸들, 플래그, 상수 정의
- [Gfx.h](JamEngine/Gfx.h) / [GfxBase.h](JamEngine/GfxBase.h) — 저수준 D3D11 래퍼
- [Graphics.h](JamEngine/Graphics.h) — 고수준 렌더링 API. `ePipelineDirtyFlags`(64비트) 기반 Dirty Flag로 파이프라인 상태를 관리한다.

### RenderGraph
- [RenderGraph.h](JamEngine/RenderGraph.h) — Pass 기반 렌더링 파이프라인 그래프
- Graphics Pass(프로그램 기반, RTV+DSV)와 Compute Pass를 지원한다.
- Pass 간 리소스 의존성을 자동 추적하고, 같은 Pass 내 SRV/RTV 충돌을 `Validate_`로 검증한다.

### Scene & ECS (EnTT)
- [Scene.h](JamEngine/Scene.h) — 씬 관리, JSON 직렬화, Deferred 엔티티 생성/삭제
- [Entity.h](JamEngine/Entity.h) — ECS 엔티티 래퍼 (UID + `entt::entity`)
- [ComponentMetadata.h](JamEngine/ComponentMetadata.h) — 런타임 컴포넌트 메타정보

### Asset Management
- [AssetManager.h](JamEngine/AssetManager.h) — 텍스처, 메시, 머티리얼, 모델 로딩/캐싱
- [AssimpImporter.h](JamEngine/AssimpImporter.h) — Assimp 기반 모델 임포트
- [FileWatcher.cpp](JamEngine/FileWatcher.cpp) — 자산 변경 감지 및 핫 리로딩

### Environment & Lighting
`Environment` 구조체는 서브 구조체로 구성된다 ([REFACTOR_GUIDE.md](REFACTOR_GUIDE.md) 참고):
```
env.dirLight.*      // 방향광 + CSM 섀도우
env.ibl.*           // Image-Based Lighting
env.skybox.*        // 스카이박스
env.ssao.*          // SSAO
env.fog.*           // 안개
env.toneMapping.*   // 톤 매핑
env.bloom.*         // 블룸
```

### Memory
- `Ref<T>` — 참조 카운팅 스마트 포인터
- `Scoped<T>` — 배타적 소유권
- [HandleAllocator.h](JamEngine/HandleAllocator.h) — 그래픽 리소스 핸들 관리

## 코딩 표준 요약

- **struct vs class**: 불변성(invariance)이 있으면 `class`, 외부에서 불변성을 보장할 수 있으면 `struct`
- **포인터 vs 참조**: 기본은 참조. `nullptr` 가능성, 배열, 출력 매개변수일 때만 포인터
- **정수형**: 기본 `Int32`. `UInt32`는 비트마스크 연산에만 사용
- **예외**: 사용 금지. 생성자는 Init 패턴으로 대체
- **Assert**: 디버그 빌드에서만 활성화 — 릴리즈에서 의존하는 코드 금지
- **에러코드**: public 함수의 입력값 검증에 사용
- 그 외는 현재 코드 흐름에 맞게 코딩 표준을 엄격하게 따름.

## 코드 포맷

`.clang-format` 파일 사용. 주요 설정:
- `IndentWidth: 4`, `ColumnLimit: 0`
- 네임스페이스/클래스/함수 앞에서 중괄호 줄바꿈
- `NamespaceIndentation: Inner`

## 역할부여
너는 리드 엔진 개발자이며, 렌더링 및 수학 라이브러리 개발에 있어서 통달한 상태야. 여기에 전문적인 지식을 제공해야해.
항상 실시간 성능을 고려해야하지.