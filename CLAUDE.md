# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 빌드

Visual Studio 2026 (v145) 솔루션 기반 (MSBuild). CMake 사용 안 함.

```
JamEngine.sln          # 메인 솔루션
```

**빌드 구성:**
- `Debug|x64`
- `Release|x64`

**빌드 출력 경로:** `Build/x64/$(Configuration)/$(MSBuildProjectName)/`

## 프로젝트 구성

| 프로젝트 | 역할 |
|----------|------|
| `JamEngine` | 엔진 정적 라이브러리 |
| `SandboxApp` | 샌드박스 |
| `JamTest` | Google Test 기반 단위 및 회귀 테스트 |
| `JamBenchmark` | Google Benchmark 기반 수학 연산 벤치마크 |

## 테스트 실행

`JamTest` 프로젝트를 빌드한 후 `Build/x64/Debug/JamTest/JamTest.exe`를 실행한다. 그래픽 창 없이 `RUN_ALL_TESTS()`를 호출하며 실패 시 종료 코드로 알린다.

전체 빌드 및 실행 명령은 [README.md](README.md)를 참고한다. 예제 실행 검증은 `scripts/Test-Examples.ps1 -Configuration Debug`로 수행한다. 기본값은 스크립트의 `$devScenes`(개발 중인 장면)만 검사하며, 공용 코드를 바꿨거나 배포 전에는 `-All`로 Debug와 Release 전체를 검증한다.

## 아키텍처 개요

### Graphics (D3D11 백엔드)
- [GraphicsBase.h](JamEngine/GraphicsBase.h) — 핸들, 플래그, 상수 정의
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
- [ModelImporter.h](JamEngine/ModelImporter.h) — Assimp 기반 모델 임포트
- [FileWatcher.cpp](JamEngine/FileWatcher.cpp) — 자산 변경 감지 및 핫 리로딩

### Environment & Lighting
`Environment` 구조체는 다음 서브 구조체로 구성된다:
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

## 네이밍 컨벤션

| 대상 | 규칙 | 예시 |
|------|------|------|
| 클래스/구조체 | PascalCase | `AssetManager`, `BasicBlob` |
| 인터페이스 | `I` 접두사 + PascalCase | `IAsset` |
| 함수 (public/protected) | PascalCase | `GetInstance()`, `LoadMesh()` |
| 함수 (private) | PascalCase + 후행 `_` | `OnWindowClose_()`, `CoreInit_()` |
| 지역변수 | PascalCase | `MeshCount`, `VertexBuffer` |
| 매개변수 | `_` 접두사 + camelCase | `_size`, `_pData`, `_byteWidth` |
| 멤버변수 | `m_` 접두사 + camelCase | `m_appName`, `m_bRunning` |
| 정적 멤버변수 | `s_` 접두사 + camelCase | `s_pInstance` |
| constexpr 상수 | `k` 접두사 + PascalCase | `kZero`, `kIdentity` |
| enum 타입 | `e` 접두사 + PascalCase | `eAppCoreFlags`, `eLogLevel` |
| enum 값 | PascalCase | `SceneManager`, `LeftBottomNear` |
| 매크로 | UPPER_SNAKE_CASE | `JAM_ASSERT`, `JAM_UNUSED` |
| 파일명 | PascalCase | `AssetManager.h`, `GraphicsBase.h` |

### 클래스 멤버 선언 순서

```cpp
class Foo
{
public:
    // 1. 복사/이동 삭제 (= delete)
    // 2. 정적 팩토리/싱글톤 메서드
    // 3. 공개 메서드

protected:
    // 4. 가상 메서드

private:
    // 5. 비공개 메서드 (후행 _)
    // 6. 멤버변수 (메서드보다 뒤)
};
```

### 기타 규칙

- **헤더 가드**: `#pragma once` 전용 (`#ifndef` 사용 안 함)
- **포인터/참조 선언**: 타입에 붙임 — `Type*`, `Type&`
- **타입 별칭**: `typedef` 사용 안 함, `using` 전용
- **매크로 vs constexpr**: 상수/인라인 계산은 `constexpr` 우선, 매크로는 디버그/진입점/컴포넌트 등록용만
- **[[nodiscard]]**: 거의 모든 getter에 필수
- **bool getter**: `Is` 접두사 — `IsRunning()`, `IsValid()`
- **템플릿**: C++20 Concept(`requires`) 적극 활용, `if constexpr` 사용
- **네임스페이스**: 주 네임스페이스 `Jam`, 내부 구현은 `Jam::Details`
- **초기화 리스트**: `:` 후 줄바꿈 + `, ` 들여쓰기 형식

## 코드 포맷

`.clang-format` 파일 사용. 주요 설정:
- `IndentWidth: 4`, `ColumnLimit: 0`
- 네임스페이스/클래스/함수 앞에서 중괄호 줄바꿈
- `NamespaceIndentation: Inner`

## 역할부여
너는 리드 엔진 개발자이며, 렌더링 및 수학 라이브러리 개발에 있어서 통달한 상태야. 여기에 전문적인 지식을 제공해야해.
항상 실시간 성능을 고려해야하지.
