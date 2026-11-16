<div align="center">

# JamEngine

**D3D11 기반 C++20 게임 엔진 프레임워크**

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4)
![Direct3D 11](https://img.shields.io/badge/Direct3D-11-5E9C36)
![HLSL](https://img.shields.io/badge/Shader-HLSL-8055A2)

<p align="center"><img src="Samples/PBRScene/images/overview.png" alt="IBL과 다양한 재질을 적용한 PBR 렌더링 장면" width="960"></p>

</div>

## Contents

- [프로젝트 소개](#프로젝트-소개)
- [프레임워크 구성](#프레임워크-구성)
- [렌더링 샘플](#렌더링-샘플)
- [빌드 및 실행](#빌드-및-실행)
- [디렉토리 구조](#디렉토리-구조)

<br>

## 프로젝트 소개

부경대학교 캡스톤디자인 경진대회 출품을 위해 9개월간 개발한 **D3D11 기반 C++20 게임 엔진 프레임워크**입니다.
이 저장소는 Jam Engine 프로젝트의 메인 저장소로, 게임 개발에 필요한 범용 도구와 렌더링 기능을 포함합니다.

특히 렌더링 구현에 중점을 두었으며, `SandboxApp`에 다양한 그래픽스 기법을 적용한 예제를 담았습니다.

<br>

## 프레임워크 구성

### 애플리케이션과 씬

[`Application`](JamEngine/Application.h)이 창과 하위 시스템을 초기화하고 메인 루프를 실행합니다.
프레임마다 시간을 갱신한 뒤 업데이트, 렌더링, ImGui 출력과 화면 표시를 처리합니다.
애플리케이션은 초기화와 종료, 업데이트와 렌더링 함수를 재정의해 동작을 추가할 수 있습니다.

[`SceneManager`](JamEngine/SceneManager.h)가 활성 씬과 전환을 관리합니다.
각 [`Scene`](JamEngine/Scene.h)은 진입·종료, 업데이트, 렌더링과 이벤트 처리를 구현합니다.
SandboxApp의 예제들도 이 인터페이스를 사용하므로, 씬을 바꾸어도 애플리케이션 루프와 공통 시스템은 그대로 사용합니다.

### 엔티티·컴포넌트와 자산

씬은 EnTT 레지스트리를 가지고, [`Entity`](JamEngine/Entity.h)를 통해 컴포넌트를 추가하거나 조회합니다.
부모·자식 관계의 씬 그래프와 로컬·월드 변환을 함께 관리하며, 모델의 노드 구조로부터 엔티티를 생성할 수도 있습니다.

[`AssetManager`](JamEngine/AssetManager.h)는 모델, 메시, 재질과 텍스처를 종류와 경로로 관리합니다.
파일에서 읽은 모델과 이미지를 렌더링에 필요한 데이터로 변환하고, 씬에서 참조해 사용합니다.

### 그래픽스와 렌더링 패스

[`Graphics`](JamEngine/Graphics.h)는 D3D11의 버퍼, 텍스처, 셰이더와 렌더링 상태를 핸들로 관리하고 드로우·디스패치를 제출합니다.
씬에서는 이 인터페이스를 사용해 렌더링에 필요한 GPU 자원을 만들고 사용합니다.

여러 단계로 나뉘는 렌더링은 [`RenderGraph`](JamEngine/RenderGraph.h)에 패스와 자원 사용 관계를 등록해 구성합니다.
각 패스에 셰이더, 출력 대상, 읽고 쓸 자원과 실행 함수를 지정합니다.
디퍼드 조명이나 화면 공간 후처리처럼 앞 단계의 결과를 다음 단계에서 읽는 작업을 연결할 수 있습니다.

<br>

## 렌더링 샘플

### SimpleScene

기본적인 렌더링 테스트입니다. 스탠포드 버니 모델과 큐브, 구를 렌더링합니다.
모델의 정점에 월드·뷰·투영 변환을 적용하고, 표면의 노멀을 이용해 조명을 계산합니다.

![스탠포드 버니와 기본 도형 렌더링](Samples/SimpleScene/images/rendering.png)

### InstancingScene

같은 큐브를 반복해서 렌더링하며 인스턴싱 사용 여부에 따른 성능을 비교합니다.
일반 렌더링은 큐브마다 변환 행렬을 갱신하고 드로우콜을 호출합니다.
인스턴싱은 각 큐브의 변환 행렬을 인스턴스 버퍼에 모아 한 번의 드로우콜로 처리하므로, CPU의 반복적인 제출 비용을 줄일 수 있습니다.

![65,536개 큐브의 인스턴싱 렌더링](Samples/InstancingScene/images/cubes.png)

큐브 65,536개 기준 인스턴싱 사용 시 550–600 FPS, 미사용 시 190–210 FPS입니다.
각 범위의 중간값으로 비교하면 약 187.5%의 FPS 향상입니다.

### MultiVertexStreamScene

정점 속성을 여러 버텍스 버퍼로 나누어 전달하는 테스트입니다.
위치는 0번 슬롯, 색상은 1번 슬롯에 바인딩합니다.
입력 레이아웃에서 각 속성의 슬롯을 지정하면, 서로 다른 버퍼의 데이터를 하나의 정점 입력으로 사용할 수 있습니다.

![위치와 색상을 별도 버퍼로 전달한 큐브](Samples/MultiVertexStreamScene/images/vertex-streams.png)

### TextureScene

알베도, 노멀, AO 텍스처를 적용한 기본적인 텍스처 매핑 테스트입니다.
알베도는 표면의 색상을 정하고, 노멀 맵은 조명 계산에 사용하는 노멀을 바꿔 작은 굴곡을 표현합니다.
AO는 틈이나 접촉 부위처럼 빛이 덜 닿는 부분을 어둡게 표현합니다.

기둥과 동상 모델에 텍스처를 적용했습니다. 각 텍스처를 켜고 끄면서 표면의 색과 굴곡, 음영의 차이를 볼 수 있습니다.

![기둥과 동상에 적용한 알베도·노멀·AO 텍스처](Samples/TextureScene/images/texture-mapping.png)

### PBRScene

Metallic과 Roughness를 사용하는 물리 기반 렌더링 테스트입니다.
Metallic은 금속성에 따른 반사 특성을, Roughness는 표면의 거칠기에 따른 반사의 선명도를 조절합니다.
구와 모델에 서로 다른 재질을 적용해 그 차이를 표현했습니다.

![다양한 재질을 적용한 PBR 장면](Samples/PBRScene/images/overview.png)

IBL(Image-Based Lighting)을 적용해 주변 환경의 조명도 재질에 반영합니다.
확산광에는 Irradiance 맵을 사용하고, 정반사는 Roughness에 따라 필터링된 환경맵과 BRDF LUT로 근사합니다.
아래 사진처럼 환경이 바뀌면 같은 모델에서도 반사되는 색과 밝기가 달라집니다.

| 밝은 환경광 | 따뜻한 환경광 |
|:---:|:---:|
| ![밝은 환경에서의 동상 반사](Samples/PBRScene/images/statue-bright-environment.png) | ![따뜻한 환경에서의 동상 반사](Samples/PBRScene/images/statue-warm-environment.png) |

Studio, Courtyard, Church 환경을 선택할 수 있으며, 배경 표시도 따로 조절할 수 있습니다.

### DeferredScene

표면의 색상과 노멀, 깊이를 먼저 저장하고, 저장한 정보를 이용해 조명을 계산하는 디퍼드 렌더링 테스트입니다.

![최종 렌더링과 Albedo·Normal·Lighting 버퍼](Samples/DeferredScene/images/color-buffers.png)

오른쪽에는 위에서부터 Albedo, Normal, Lighting 버퍼를 표시했습니다.
Albedo는 표면의 색상, Normal은 표면의 방향, Lighting은 조명 계산 결과입니다.
저장한 노멀과 깊이로 조명을 계산하고, 마지막에 표면 색상과 조명 결과를 합성합니다.

Scissor Rect를 이용해 각 광원의 영향 범위를 둘러싼 화면 영역에서만 픽셀을 처리하도록 최적화했습니다.
광원의 영향이 화면 일부에만 미칠 때 불필요한 조명 계산을 줄일 수 있습니다.

| 다수의 점광원 | Scissor Rect 시각화 |
|:---:|:---:|
| ![512개 점광원의 조명 결과](Samples/DeferredScene/images/point-lights-512.png) | ![26개 점광원의 Scissor Rect](Samples/DeferredScene/images/scissor-rects.png) |

왼쪽은 점광원 512개의 조명 결과이고, 오른쪽은 점광원 26개의 Scissor Rect를 표시한 화면입니다.
색 사각형은 각 광원의 픽셀 처리 범위를 나타냅니다.

점광원 2,048개에 Scissor Rect를 적용한 상태에서 약 240 FPS를 유지했습니다. 아래 화면의 평균 FPS는 239.84입니다.

![점광원 2,048개 렌더링 성능: 평균 239.84 FPS](Samples/DeferredScene/images/benchmark-2048-lights.png)

### ShadowScene

광원에서 바라본 깊이를 저장한 뒤, 표면의 깊이와 비교하는 섀도 매핑을 구현했습니다.
광원과 해당 표면 사이에 다른 물체가 있으면 해당 위치를 그림자로 처리합니다.

| 기둥과 동상의 그림자 | 모델 주변의 그림자 경계 |
|:---:|:---:|
| ![기둥과 동상이 드리운 그림자](Samples/ShadowScene/images/columns-and-statues.png) | ![구와 용 모델의 그림자](Samples/ShadowScene/images/shadow-detail.png) |

점광원에는 여섯 방향의 깊이를 2D 텍스처 배열에 저장하는 Omni Shadow Mapping을 사용합니다.
스팟라이트는 광원 시점의 깊이를 한 장 저장합니다.
방향광에는 카메라의 가시 거리를 네 구간으로 나누는 Cascaded Shadow Mapping을 사용해 가까운 영역에 더 많은 해상도를 배분합니다.

Hard는 깊이를 한 번 비교하고, PCF는 주변의 비교 결과를 평균내어 경계를 부드럽게 만듭니다. PCF는 3×3, 5×5, 7×7을 지원합니다.
VSM은 깊이와 깊이의 제곱을 저장하고, 두 값으로 구한 분산을 이용해 그림자를 근사합니다.
광원 종류, 필터, 섀도 맵 해상도를 바꾸며 결과를 비교할 수 있습니다.

### StencilScene

스텐실 버퍼를 이용한 평면 반사와 아웃라인 테스트입니다.
스텐실에 기록한 값을 조건으로 사용해 오브젝트가 그려지는 영역을 제한합니다.

| 평면 반사와 아웃라인 | 모델의 외곽선 |
|:---:|:---:|
| ![바닥에 반사된 오브젝트와 아웃라인](Samples/StencilScene/images/reflection-and-outline.png) | ![버니 모델의 아웃라인과 반사](Samples/StencilScene/images/bunny-outline.png) |

평면 반사는 y=0 기준의 반사 행렬로 오브젝트를 한 번 더 렌더링합니다.
거울 영역에 스텐실을 기록해 반사된 오브젝트가 그 안에서만 보이도록 합니다.

아웃라인은 원래 오브젝트를 그리며 스텐실에 1을 기록합니다.
이후 정점을 노멀 방향으로 밀어 확장한 모델을 그리되, 스텐실이 1인 부분은 제외합니다.
원래 모델의 바깥으로 나온 부분만 남아 외곽선이 됩니다.

### CullingScene

옥트리 기반 프러스텀 컬링 테스트입니다.
오브젝트를 공간별로 나누어 저장하고, 옥트리 노드의 영역을 카메라 프러스텀과 비교합니다.
노드가 프러스텀 밖에 있으면 그 안의 오브젝트를 한꺼번에 제외할 수 있어, 모든 오브젝트를 하나씩 검사하는 비용을 줄입니다.

![옥트리의 공간 분할과 탑뷰의 카메라 프러스텀](Samples/CullingScene/images/spatial-partition-04.png)

박스는 옥트리의 공간 분할을 나타냅니다. 탑뷰에서는 카메라 프러스텀과 오브젝트의 분포를 함께 볼 수 있습니다.

오브젝트 20만 개를 대상으로 컬링 미사용, 선형 검사, 옥트리를 비교했습니다.
아래 평균 FPS는 각 측정 화면의 Avg. Frame Rate 값입니다.

| 컬링 미사용 | 선형 검사 | 옥트리 |
|:---:|:---:|:---:|
| ![컬링 미사용 측정 화면](Samples/CullingScene/images/benchmark-none.png) | ![선형 컬링 측정 화면](Samples/CullingScene/images/benchmark-linear.png) | ![옥트리 컬링 측정 화면](Samples/CullingScene/images/benchmark-octree.png) |
| **71.44 FPS** | **86.92 FPS** | **97.20 FPS** |

옥트리 적용 시 미사용 대비 약 36.1%, 선형 검사 대비 약 11.8%의 FPS 향상이 나타났습니다.

### TerrainScene

Perlin Noise를 여러 옥타브로 합쳐 지형을 생성합니다.
CPU에서 높이맵을 만들고, 버텍스 셰이더에서 높이를 읽어 격자 정점을 위아래로 이동시킵니다.

![노이즈 높이맵으로 생성한 지형](Samples/TerrainScene/images/terrain.png)

인접한 높이의 차이로 노멀을 계산해 지형의 경사에 맞게 조명을 적용합니다.
높이와 경사에 따라 풀, 바위, 눈, 흙 텍스처를 섞습니다.
높이와 노멀을 따로 표시하면 지형의 형태와 조명에 사용하는 데이터를 살펴볼 수 있습니다.

| 높이 시각화 | 노멀 시각화 |
|:---:|:---:|
| ![높이를 명암으로 표시한 지형](Samples/TerrainScene/images/height.png) | ![노멀 방향을 색상으로 표시한 지형](Samples/TerrainScene/images/normals.png) |

### SSAOScene

깊이와 노멀 버퍼로 주변광 차폐를 계산하는 SSAO(Screen Space Ambient Occlusion) 테스트입니다.
주변 표면에 의해 빛이 덜 닿는 부분을 어둡게 만들어 틈과 접촉 부위의 음영을 표현합니다.

| 최종 장면 | AO 결과 |
|:---:|:---:|
| ![SSAO가 적용된 Sponza 장면](Samples/SSAOScene/images/scene.png) | ![Sponza의 AO만 표시한 화면](Samples/SSAOScene/images/ambient-occlusion.png) |

깊이에서 현재 픽셀의 뷰 공간 위치를 복원하고, 노멀 방향의 반구 안에 샘플을 배치합니다.
샘플을 화면에 투영해 깊이와 비교하며 주변 표면에 가려지는 정도를 구합니다.
노이즈 텍스처로 샘플 방향을 회전시키고 블러로 결과를 정리한 뒤, 최종 조명의 주변광에만 적용합니다.

### SSRScene

화면에 렌더링한 정보를 이용해 반사를 만드는 SSR(Screen Space Reflection) 테스트입니다.
반사 표면에서 레이를 진행시키며 깊이 버퍼와 교차하는 위치를 찾고, 해당 위치의 색상을 반사색으로 사용합니다.

| 바닥과 구의 반사 | 다른 시점의 반사 |
|:---:|:---:|
| ![바닥과 구에 적용한 화면 공간 반사](Samples/SSRScene/images/reflections.png) | ![위에서 바라본 모델과 바닥의 반사](Samples/SSRScene/images/reflections-overhead.png) |

교차 후보의 표면 방향을 검사하고, 색상을 섞을 때 깊이와 노멀을 함께 확인해 다른 물체의 색이 섞이는 현상을 줄입니다.
화면 가장자리와 교차가 불확실한 부분에서는 반사를 약하게 처리합니다.

화면 밖이나 다른 물체 뒤에 가려진 정보는 사용할 수 없다는 한계가 있습니다.
Fallback을 켜면 반사를 찾지 못한 부분을 하늘·지면 방향에 따라 계산한 배경색으로 채웁니다.

### ParticleScene

컴퓨트 셰이더를 이용한 GPU 파티클 테스트입니다.
RWStructuredBuffer에 파티클의 위치, 속도, 회전, 수명을 저장하고 GPU에서 병렬로 갱신합니다.
버텍스 셰이더가 같은 버퍼를 읽어 렌더링하므로 시뮬레이션 결과를 CPU로 가져올 필요가 없습니다.

| 눈 | 색종이 |
|:---:|:---:|
| ![눈 파티클](Samples/ParticleScene/images/snow.png) | ![색종이 파티클](Samples/ParticleScene/images/confetti.png) |

파티클 하나는 두 삼각형으로 만든 사각형이며, 전체 파티클을 인스턴싱으로 렌더링합니다.
정점 번호로 사각형의 모양을 만들고, 인스턴스 번호로 해당 파티클의 데이터를 읽습니다.
파티클 개수가 바뀌면 CPU에서 간접 드로우 인자를 갱신하고, DrawInstancedIndirect 한 번으로 제출합니다.

파티클 10만 개에서 약 2,400 FPS입니다.

### MousePickingScene

오브젝트 ID 버퍼를 이용한 마우스 피킹 테스트입니다.
색상과 ID를 서로 다른 렌더 타깃에 동시에 기록하고, 클릭한 위치의 ID 한 픽셀만 CPU로 읽어 선택합니다.
ID는 R32_UInt 형식으로 저장하며, 배경과 바닥의 ID는 0으로 두어 클릭 시 선택이 해제되도록 합니다.

| 오브젝트 ID 시각화 | 선택한 오브젝트의 아웃라인 |
|:---:|:---:|
| ![오브젝트마다 다른 색으로 표시한 ID 버퍼](Samples/MousePickingScene/images/object-ids.png) | ![선택한 구의 금색 아웃라인](Samples/MousePickingScene/images/selection-outline.png) |

아웃라인은 후처리에서 ID 버퍼를 읽어 그립니다.
선택된 ID의 픽셀 중 주변에 다른 ID가 있는 픽셀을 금색으로 칠하므로, 외곽선은 물체의 안쪽 경계에 생깁니다.
주변 픽셀의 검사 범위로 선의 두께를 조절합니다.

### SkinningScene

선형 블렌딩 스키닝을 이용한 캐릭터 애니메이션 테스트입니다.
Idle, Walk, Slash를 재생하고, 상태가 바뀔 때 이전 자세와 새 자세를 블렌딩해 자연스럽게 연결합니다.

| 캐릭터 애니메이션 | 자세에 따른 메시 변형 |
|:---:|:---:|
| ![여러 캐릭터의 애니메이션 재생](Samples/SkinningScene/images/animation.png) | ![서로 다른 자세의 캐릭터](Samples/SkinningScene/images/poses.png) |

CPU에서 조인트별 스키닝 행렬을 계산해 상수버퍼로 전달합니다.
버텍스 셰이더에서는 정점마다 저장된 조인트 번호와 가중치로 행렬을 섞어 메시를 변형합니다.
WASD로 이동하고 Space로 Slash를 재생합니다.

성능 수치는 개발자 PC 기준입니다.

ComputeScene에는 별도로 17개의 컴퓨트 셰이더 예제가 있습니다. 실행 중 `Scenes` 창에서 선택할 수 있습니다.

<br>

## 빌드 및 실행

Windows x64에서 C++20으로 빌드합니다. Visual Studio 2026의 C++ 데스크톱 개발 도구, MSVC `v145` 도구 집합, Windows SDK와 vcpkg가 필요합니다.

1. 저장소를 받습니다.

   ```powershell
   git clone https://github.com/JuggernautSensei/JamEngine.git
   cd JamEngine
   ```

2. vcpkg에서 의존성을 설치하고 MSBuild에 연결합니다.

   ```powershell
   vcpkg install assimp benchmark directxmesh "directxtex[dx11,openexr]" directxtk eastl efsw entt fmt gtest lunasvg spdlog unordered-dense yyjson --triplet x64-windows
   vcpkg integrate install
   ```

3. `JamEngine.sln`을 열어 `SandboxApp`을 시작 프로젝트로 지정하거나, 저장소 루트에서 빌드 스크립트를 실행합니다.

   ```powershell
   ./scripts/Build.ps1 -Configuration All
   ./Build/x64/Debug/SandboxApp/SandboxApp.exe
   ```

지원 구성은 `Debug|x64`, `Release|x64`입니다. 실행 파일은 `Build/x64/<구성>/<프로젝트>/`에 생성되며, Sandbox 리소스는 빌드할 때 실행 파일 옆으로 복사됩니다.

실행하면 SimpleScene으로 시작합니다. 왼쪽 위 `Scenes` 창에서 씬을 선택하고, 각 씬의 ImGui 창에서 설정을 조절합니다.

| 조작 | 키 |
|---|---|
| 비행 카메라 이동 | WASD / QE |
| 비행 카메라 시점 회전 | 마우스 오른쪽 버튼을 누른 채 드래그 |
| 오브젝트 선택 / 선택 해제 | MousePickingScene에서 물체 / 배경 왼쪽 클릭 |
| 캐릭터 이동 / 베기 | SkinningScene에서 WASD / Space |

외부 모델·텍스처·환경맵의 출처와 라이선스는 [ASSETS.md](SandboxApp/Resources/ASSETS.md)에 있습니다.

<br>

## 디렉토리 구조

| 경로 | 내용 |
|---|---|
| `JamEngine/` | 애플리케이션, 씬, 자산, 그래픽스와 공통 기능 |
| `SandboxApp/` | 엔진을 사용하는 그래픽스 예제 |
| `SandboxApp/Resources/` | 모델, 텍스처와 HLSL 셰이더 |
| `Samples/` | 씬별 기술 설명과 스크린샷 |
| `JamBenchmark/` | 성능 측정 |
| `scripts/` | 빌드 스크립트 |
| `Build/` | 빌드 출력과 중간 파일 |
