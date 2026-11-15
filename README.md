# JamEngine

Windows / Direct3D 11 기반 C++20 엔진입니다. `JamEngine.sln`에서 엔진, 예제, 테스트, 벤치마크를 함께 빌드합니다.

## 빌드

Visual Studio 2026의 **Desktop development with C++**, v145 도구 집합, Windows SDK가 필요합니다. vcpkg classic mode에서 다음 패키지를 `x64-windows`로 설치하고 MSBuild에 연결합니다.

```powershell
vcpkg install assimp benchmark directxmesh "directxtex[dx11,openexr]" directxtk eastl efsw entt fmt gtest lunasvg spdlog unordered-dense yyjson --triplet x64-windows
vcpkg integrate install
./scripts/Build.ps1 -Configuration All
```

지원 구성은 `Debug|x64`, `Release|x64`입니다. 출력은 `Build/x64/<구성>/<프로젝트>/`, 중간 파일은 `Build/Intermediate/`에 생성됩니다. `Directory.Build.props`와 `Directory.Build.targets`에서 공통 경로와 링크 의존성을 관리합니다. ImGui와 ImGuizmo는 저장소에 포함된 소스를 사용합니다.

## 실행

```powershell
# 기본 SimpleScene 실행. 실행 파일 옆의 Resources를 자동으로 사용합니다.
./Build/x64/Debug/SandboxApp/SandboxApp.exe

# 장면 목록 / 특정 장면 / Compute 데모
./Build/x64/Debug/SandboxApp/SandboxApp.exe --list-scenes
./Build/x64/Debug/SandboxApp/SandboxApp.exe --scene ShadowMappingScene
./Build/x64/Debug/SandboxApp/SandboxApp.exe --scene ComputeScene --demo 17
./Build/x64/Debug/SandboxApp/SandboxApp.exe --scene ParticleScene --demo 2 --particles 6000
./Build/x64/Debug/SandboxApp/SandboxApp.exe --scene MousePickingScene
```

장면은 `SimpleScene`, `InstancingScene`, `MultiVertexStreamScene`, `TextureMappingScene`, `PBRScene`, `DeferredScene`, `DebugNormalScene`, `ShadowMappingScene`, `StencilScene`, `CullingScene`, `TerrainScene`, `SSAOScene`, `SSRScene`, `ParticleScene`, `MousePickingScene`, `ComputeScene`, `SkinningScene`입니다. Compute에는 17개 세부 데모가 있습니다. 일반 실행에서는 GUI로 데모를 선택할 수 있습니다.

`--frames N`은 지정한 프레임 수 후 정상 종료합니다. Sandbox에서는 `--screenshot PATH`를 함께 주면 마지막 장면 렌더링을 PNG로 저장합니다. `--demo`는 ComputeScene에서 1–17, ParticleScene에서 1(눈)·2(색종이)를 선택합니다. ParticleScene의 `--particles N`은 0–100000개를 지정합니다. GUI에서 효과·입자 수·바람·크기·낙하 속도·일시정지·재시작을 조절할 수 있습니다. 폐기된 에디터 모드는 제거했습니다.

ParticleScene은 고정 크기 일반 RW 버퍼를 컴퓨트 셰이더에서 갱신하고 인스턴싱으로 그립니다. Append/Consume이나 입자 데이터의 CPU 왕복 전송을 사용하지 않습니다. `./scripts/Test-Particles.ps1`은 실제 셰이더를 WARP에서 실행해 개수 경계와 입자 생명주기를 검사합니다.

MousePickingScene은 색상과 정수 ID를 MRT로 함께 출력합니다. ID 버퍼는 매 프레임 0으로 클리어하고 물체 ID는 1부터 사용합니다. 왼쪽 클릭 시 해당 픽셀 하나를 읽어 선택하며, 배경이나 바닥을 클릭하면 선택을 해제합니다. GUI에서 ID 버퍼 보기·외곽선 두께·애니메이션을 조절합니다. `--pick-x X --pick-y Y`는 클라이언트 좌표에 한 번의 선택 요청을 예약하므로 `--frames`·`--screenshot`과 함께 결과를 재현할 수 있습니다. `--show-ids`는 ID 시각화로 시작합니다. CPU 읽기는 선택 요청 때만 수행하며 GPU 동기화가 발생할 수 있습니다.

## 검증

```powershell
./Build/x64/Debug/JamTest/JamTest.exe
./scripts/Test-SSRTrace.ps1                                  # 창 없이 SSR 셰이더 회귀 검사 (WARP)
./scripts/Test-Picking.ps1                                   # MRT·깊이·정수 ID·1픽셀 읽기·외곽선 검사 (WARP)
./scripts/Test-Examples.ps1 -Configuration Debug              # 개발 중인 장면만 ($devScenes)
./scripts/Test-Examples.ps1 -Configuration Debug -Scene PBRScene   # 특정 장면
./scripts/Test-Examples.ps1 -Configuration Debug -All          # 전체 (공용 코드 변경 후, 배포 전)
./scripts/Test-Examples.ps1 -Configuration Release -All
./Build/x64/Release/JamBenchmark/JamBenchmark.exe --benchmark_min_time=0.001s
```

SkinningScene은 Mixamo FBX를 CPU에서 샘플링해 GPU 선형 블렌드 스키닝으로 그립니다. WASD로 이동(Idle↔Walk, 0.2초 크로스페이드)하고 Space로 베기를 한 번 재생합니다. Idle/Walk/Slash 클립은 본 이름으로 메인 스켈레톤에 리타겟팅하며, Hips의 가로 이동은 고정합니다.

예제 검증 스크립트의 기본 대상은 개발 중인 MousePickingScene의 기본·선택·배경·ID 보기 네 경우입니다. 선택 요청은 예상 ID까지 검사하고, 요청이 없는 실행에서는 읽기가 발생하지 않는지 확인합니다. `-All`은 17개 장면의 총 37개 실행 항목(Compute 17개 데모, Particle 2개 프리셋, MousePicking 4개 경우 포함)을 검사해 종료 코드, 오류 로그, PNG 생성을 확인합니다. 결과는 `Build/Validation/<구성>/`에 남깁니다. 테스트 자동화에서는 환경 변수 `JAM_NO_ERROR_DIALOG=1`로 크래시 메시지 상자를 끌 수 있으며, 오류 로그와 비정상 종료는 유지됩니다.

SSR 전용 검사는 실제 셰이더를 Debug·Release 옵션으로 컴파일하고, 얇은 면 누락·물체 겹침 경계·깊이 오검출·노멀 방향에 따른 누락·픽셀 경계의 색 조회를 검사합니다. SSR은 화면 픽셀을 따라 깊이 구간을 비교하며, 앞 물체에 완전히 가려진 면이나 화면 밖 물체의 정보는 단일 G-Buffer에서 복원할 수 없습니다.

## 코드 구조

| 경로 | 역할 |
|---|---|
| `JamEngine/` | 엔진 정적 라이브러리. 메모리·문자열·수학·파일·플랫폼·그래픽스·씬·자산·공용 GUI |
| `JamEngine/Vendor/` | 포함된 ImGui, ImGuizmo, RenderDoc 등의 외부 코드 |
| `SandboxApp/` | 장면별 그래픽스 예제와 `Resources/` 원본 자산 |
| `JamTest/` | Google Test 테스트와 복구 오류에 대한 회귀 테스트 |
| `JamBenchmark/` | Google Benchmark 성능 측정 |
| `scripts/` | 빌드 및 예제 실행 검증 |
| `Build/` | 생성 파일 전체. Git에서 제외 |

`Application`이 플랫폼·Graphics·Input과 선택한 하위 시스템을 초기화하고 갱신합니다. `SceneManager`가 `Scene`을 실행하며, 각 예제는 `RenderGraph`에 패스와 자원을 선언합니다. `Graphics`가 D3D11 자원, 상태 캐시, 드로우·디스패치를 담당합니다. 자산은 `AssetManager`, `ModelImporter`, `Image`, `ShaderBinary`를 통해 읽습니다.

복구 내용, 삭제 기준, 기존 제외 테스트의 범위는 [docs/Cleanup.md](docs/Cleanup.md)에 정리했습니다.
