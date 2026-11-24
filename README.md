<h1 align="center">JamEngine</h1>

<p align="center">Direct3D 11 기반 C++20 게임 개발 프레임워크</p>

<p align="center">
  <img src="https://img.shields.io/badge/C++-20-00599C?logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/Windows-x64-0078D4" alt="Windows x64">
  <img src="https://img.shields.io/badge/Direct3D-11-5E9C36" alt="Direct3D 11">
  <img src="https://img.shields.io/badge/Shader-HLSL-8055A2" alt="HLSL">
</p>

![IBL과 다양한 재질을 적용한 PBR 렌더링 장면](Samples/PBRScene/images/overview.png)

## Contents

* [프로젝트 소개](#프로젝트-소개)
* [에디터](#에디터)
* [컴포넌트 리플렉션](#컴포넌트-리플렉션)
* [렌더 그래프](#렌더-그래프)
* [렌더링 샘플](#렌더링-샘플)
* [빌드 및 실행](#빌드-및-실행)

## 프로젝트 소개

부경대학교 캡스톤디자인 경진대회 출품을 위해 9개월간 개발한 Direct3D 11 기반 C++20 게임 엔진 프레임워크입니다. 게임 개발에 필요한 기반을 직접 구현해 하나의 정적 라이브러리 `JamEngine`으로 제공하고, `SandboxApp`에서 이 엔진으로 만든 렌더링 샘플을 실행할 수 있습니다.

- 애플리케이션과 OS: Win32 창, 메인 루프와 씬 전환, 키보드·마우스 입력, 타이머, 로그, 파일 시스템과 파일 감시, 파일·메시지 대화상자, 커맨드라인 파서
- 수학: 벡터, 행렬, 사원수, 트랜스폼, 바운딩 볼륨, 옥트리, 난수, 16비트 부동소수점
- 렌더링: 핸들 기반 D3D11 그래픽스 API, 렌더 그래프, 셰이더·텍스처·메시·머테리얼, 카메라, 스켈레톤 애니메이션
- 씬과 에셋: EnTT 기반 엔티티·컴포넌트, 부모·자식 씬 그래프, FBX 등 모델 임포트와 엔진 전용 에셋 포맷
- GUI 에디팅: ImGui 에디터, 기즈모, 인스펙터, 파일 브라우저, 프로파일러
- 스크립트: 엔티티에 붙여 실행하는 스크립트 컴포넌트
- 리플렉션과 직렬화: 컴포넌트 리플렉션, JSON 아카이브, 씬 저장·불러오기
- 기반 유틸리티: 메모리 풀·아레나·핸들 할당자, 델리게이트·시그널·이벤트, 문자열과 해시, LRU 캐시, 실행 취소 히스토리

### 모듈과 역할

| 모듈 | 역할 |
| --- | --- |
| [`Application`](JamEngine/Application.h), [`SceneManager`](JamEngine/SceneManager.h) | 창과 서브시스템 초기화, 메인 루프, 씬 전환 |
| [`Input`](JamEngine/Input.h), [`FileSystem`](JamEngine/FileSystem.h), [`FileWatcher`](JamEngine/FileWatcher.h) | 입력 처리, 파일 입출력과 변경 감시 |
| [`Math`](JamEngine/Math.h), [`BoundingVolume`](JamEngine/BoundingVolume.h), [`Octree`](JamEngine/Octree.h) | 벡터·행렬·사원수 연산, 충돌 판정용 볼륨, 공간 분할 |
| [`Scene`](JamEngine/Scene.h), [`Entity`](JamEngine/Entity.h) | EnTT 기반 ECS, 부모·자식 씬 그래프와 로컬·월드 트랜스폼 |
| [`AssetManager`](JamEngine/AssetManager.h), [`ModelImporter`](JamEngine/ModelImporter.h) | 모델·메시·머테리얼·텍스처 관리, 외부 모델 임포트와 엔티티 계층 생성 |
| [`Graphics`](JamEngine/Graphics.h) | D3D11 리소스의 핸들 기반 관리, 드로우·디스패치 제출 |
| [`RenderGraph`](JamEngine/RenderGraph.h) | 패스 컬링, 리소스 읽기·쓰기 충돌을 해결하는 바인딩 명령 생성 |
| [`ComponentDatabase`](JamEngine/ComponentDatabase.h) | 컴포넌트 리플렉션, 타입별 함수 테이블 |
| [`JsonArchive`](JamEngine/JsonArchive.h), [`SceneSerializer`](JamEngine/SceneSerializer.h) | 컴포넌트와 씬의 직렬화·역직렬화 |
| [`ImGuiEngine`](JamEngine/ImGuiEngine.h), [`EditorWidgets`](JamEngine/EditorWidgets.h) | 에디터 UI와 인스펙터 위젯 |

단위 테스트는 `JamTest`, 수학·문자열·옥트리 벤치마크는 `JamBenchmark` 프로젝트에 있습니다.

## 에디터

ImGui로 만든 에디터입니다. 씬 계층, 씬 뷰, 인스펙터, 파일 브라우저, 프로파일러 창으로 구성됩니다.

### 엔티티 편집

![엔티티를 만들고 기즈모로 옮긴 뒤 인스펙터에서 렌더러블 컴포넌트를 추가하고 색과 크기를 바꿔 복제하는 장면](Samples/Editor/images/entity-editing.gif)

씬 계층 창에서 엔티티를 생성·복제·삭제하고 부모·자식 관계를 편집합니다. 씬 뷰에서는 기즈모로 트랜스폼을 조작하며, 월드 기준과 로컬 기준을 전환할 수 있습니다. 인스펙터의 컴포넌트 목록과 편집 패널은 [컴포넌트 리플렉션](#컴포넌트-리플렉션) 정보로 만들어집니다.

### 모델 임포트

![파일 브라우저에서 FBX 모델을 임포트해 씬에 배치하고, 텍스처를 재질 슬롯에 지정하는 장면](Samples/Editor/images/model-import.gif)

파일 브라우저에서 FBX 모델을 임포트하면 엔진 전용 포맷인 메시(`.jmsh`), 재질(`.jmat`), 모델(`.jmdl`) 파일이 생성됩니다. 모델을 씬에 배치하면 원본의 노드 구조대로 엔티티 계층이 만들어집니다. 재질 슬롯에 텍스처를 지정하면 씬 뷰에 바로 반영됩니다.

### 씬 저장과 실행

![Sponza 씬을 파일로 저장하고 빈 씬에서 다시 불러온 뒤, 카메라 컴포넌트를 붙여 게임 화면으로 전환하는 장면](Samples/Editor/images/scene-serialization.gif)

씬은 파일로 저장하고 다시 불러올 수 있습니다. Run 버튼을 누르면 카메라 컴포넌트가 붙은 엔티티의 시점으로 게임이 실행되고, Edit 버튼으로 편집 모드에 돌아옵니다.

### 스크립트

![스크립트로 움직이는 큐브를 게임 화면과 편집 화면을 오가며 확인하는 장면](Samples/Editor/images/script-play.gif)

게임 로직은 `OnStart`, `OnUpdate`, `OnStop`을 오버라이드한 스크립트 클래스로 작성하고, 스크립트 컴포넌트로 엔티티에 붙입니다. 위 장면은 방향키로 큐브를 움직이는 스크립트입니다.

## 컴포넌트 리플렉션

C++에는 런타임에 타입 정보를 조회하는 기능이 없어서, 엔진이 여러 컴포넌트를 공통으로 다루려면 타입마다 분기 코드를 써야 합니다. 컴포넌트가 늘어날 때마다 인스펙터, 씬 저장, 엔티티 복제 코드를 모두 고쳐야 하는 셈입니다. JamEngine은 컴포넌트가 메타데이터를 등록하면 엔진이 타입별 함수 테이블을 만들고, 엔진 코드는 이 테이블만 사용하도록 리플렉션을 구현했습니다.

카메라, 렌더러블, 조명, 태그 등 모든 컴포넌트가 같은 방식으로 선언됩니다. 아래는 점광원 컴포넌트의 예입니다.

```cpp
struct PointLightComponent
{
    JAM_COMPONENT_BODY(PointLightComponent);
    JAM_COMPONENT_DISPLAY_NAME("point light component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _owner);
    void DrawEditPanel(Scene* _pScene, Entity _entity);

    Vec3  radiance     = Vec3::kZero;
    float intensity    = 1.0f;
    float fallOffStart = 1.0f;
    float fallOffEnd   = 10.0f;
};
```

`JAM_COMPONENT_BODY`가 `inline static` 등록 객체를 만들고, 이 객체가 정적 초기화 시점에 [`ComponentDatabase`](JamEngine/ComponentDatabase.h)에 타입을 등록합니다. 생성, 복사, 제거, 조회 함수는 템플릿으로 만들어 `Delegate`에 저장하고, 선언한 플래그에 따라 직렬화와 편집 함수도 함께 등록합니다.

등록된 정보는 엔진 곳곳에서 쓰입니다.

| 사용처 | 리플렉션으로 하는 일 |
| --- | --- |
| 인스펙터 | 표시 이름으로 컴포넌트 추가 메뉴를 만들고, `DrawEditPanel`로 편집 패널을 그립니다. |
| 씬 저장·불러오기 | `Serializable` 컴포넌트를 순회하며 `Serialize`/`Deserialize`를 호출합니다. |
| 엔티티 복제 | 원본이 가진 컴포넌트를 찾아 복사 생성하고, 자식 엔티티까지 재귀적으로 복제합니다. [엔티티 복제](JamEngine/Scene.cpp) |
| 에디터 제약 | `NonRemoveable`, `NonCreatableOnEditor` 플래그로 제거·추가 가능 여부를 제한합니다. |

이 구조로 얻는 이득은 세 가지입니다.

- 확장성: 새 컴포넌트는 선언만 하면 인스펙터, 저장, 복제에 자동으로 연결됩니다. 엔진 코드는 수정하지 않습니다.
- 안전성: 플래그에 필요한 함수가 없으면 `static_assert`가 컴포넌트 이름과 함께 빌드를 실패시켜, 저장이나 편집 누락을 런타임 전에 잡습니다. [플래그 검증](JamEngine/ComponentMetadata.h)
- 단순함: 타입별 `if`/`switch` 분기가 사라져, 공통 처리 코드가 한 곳에 모입니다.

## 렌더 그래프

디퍼드 조명이나 SSAO처럼 여러 패스가 앞 패스의 결과를 읽는 렌더링에서는, 같은 리소스가 한 패스에서는 쓰기 대상이고 다음 패스에서는 읽기 대상이 됩니다. D3D11은 하나의 리소스를 읽기와 쓰기에 동시에 바인딩할 수 없으므로, 패스마다 이전 바인딩을 풀고 새로 묶는 코드를 직접 맞춰야 합니다. 패스를 추가하거나 순서를 바꿀 때마다 이 읽기·쓰기 충돌을 다시 따져야 해서 실수가 잦았습니다.

[`RenderGraph`](JamEngine/RenderGraph.h)에서는 패스가 읽고 쓰는 리소스를 이름으로 선언하기만 하면, 충돌 해결은 그래프가 맡습니다. 아래는 SSAO 샘플에서 G-buffer의 노멀과 깊이를 읽는 패스입니다.

```cpp
m_graph.AddRenderPass("Pass_Ssao")
    .SetFrameBuffer("FB_Ssao")
    .SetProgram("P_Ssao")
    .SetTexture("T_GNormal", eShader::PixelShader, 0)
    .SetTexture("T_GDepth", eShader::PixelShader, 1)
    .SetConstantBuffer("CB_Ssao", eShader::PixelShader, CBSsao::kSlot)
    .SetExecutor(Bind<&SSAOScene::SubmitSsao_>(this));
```

그래프는 패스 구성이 바뀔 때만 컴파일되고, 컴파일은 두 단계로 진행됩니다.

1. 패스 컬링: 최종 출력 패스와 부수 효과가 있는 패스에서 리소스 의존성을 역방향으로 탐색합니다. 도달하지 않은 패스는 결과가 쓰이지 않으므로 실행 목록에서 제외합니다.
2. 바인딩 명령 생성: 남은 패스를 순서대로 따라가며 슬롯별 바인딩 상태를 추적합니다. 쓰기로 묶인 리소스를 읽으려 하거나 읽기로 묶인 리소스에 쓰려 하면, 먼저 기존 바인딩을 해제하는 명령을 기록합니다. 이미 같은 슬롯에 묶인 리소스는 다시 바인딩하지 않습니다. 프레임 경계에서도 충돌이 없도록 추적을 두 번 돌립니다.

런타임에는 미리 만든 명령만 순서대로 실행하므로 매 프레임 의존성을 다시 계산하지 않습니다. 창 크기가 바뀌면 화면 크기에 맞춘 텍스처만 다시 만듭니다. [렌더 그래프 구현](JamEngine/RenderGraph.cpp)

## 렌더링 샘플

SandboxApp의 Scenes 창에서 샘플을 선택합니다. 모든 샘플은 같은 `Scene` 인터페이스를 구현하며, 멀티 패스 샘플은 렌더 그래프로 구성했습니다.

### InstancingScene

![65,536개 큐브의 인스턴싱 렌더링](Samples/InstancingScene/images/cubes.png)

큐브 65,536개의 월드 행렬을 인스턴스 버퍼에 모아 드로우 콜 한 번으로 그립니다. 큐브마다 드로우 콜을 호출할 때 190–210 FPS였고, 인스턴싱 적용 후 550–600 FPS로 약 2.9배 빨라졌습니다.

### PBRScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/PBRScene/images/statue-bright-environment.png" alt="밝은 환경에서의 동상 반사" width="100%"></td>
      <td align="center"><img src="Samples/PBRScene/images/statue-warm-environment.png" alt="따뜻한 환경에서의 동상 반사" width="100%"></td>
    </tr>
  </tbody>
</table>

Metallic·Roughness 워크플로의 물리 기반 렌더링(PBR)을 구현했습니다. 주변 환경 전체를 광원으로 쓰는 IBL을 적용해, 디퓨즈는 Irradiance 맵에서 가져오고, 스페큘러는 러프니스별로 필터링한 환경 맵과 BRDF 적분식을 미리 계산해 둔 BRDF LUT로 근사합니다. 그 결과 금속과 비금속, 거친 표면과 매끈한 표면이 빛을 반사하는 모습을 물리적으로 근사한 색으로 렌더링합니다.

두 그림은 같은 동상을 서로 다른 환경 맵에 둔 결과입니다. 머테리얼은 그대로인데, 주변 환경의 밝기와 색이 바뀌면 동상 표면에 맺히는 반사광의 색과 세기가 함께 달라집니다.

### DeferredScene

![G-buffer의 알베도, 노멀과 조명 누적 결과를 디버그 창으로 표시한 화면](Samples/DeferredScene/images/color-buffers.png)

지오메트리 패스에서 알베도, 노멀, 깊이를 G-buffer에 기록하고, 라이팅 패스에서 이를 읽어 광원마다 조명을 누적합니다. 오른쪽 디버그 창은 위에서부터 알베도(`T_GColor`), 노멀(`T_GNormal`), 조명 누적 결과(`T_Light`)입니다.

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/DeferredScene/images/point-lights-512.png" alt="512개 점광원의 조명 결과" width="100%"></td>
      <td align="center"><img src="Samples/DeferredScene/images/scissor-rects.png" alt="점광원마다 처리 영역을 색 사각형으로 표시한 Scissor Rect" width="100%"></td>
    </tr>
  </tbody>
</table>

광원이 많아지면 광원마다 화면 전체를 처리하는 비용이 커집니다. 점광원은 영향 반경이 정해져 있으므로 그 범위를 화면에 투영해 Scissor Rect로 지정하고, 사각형 안의 픽셀만 조명을 계산합니다. 화면 밖에 있는 광원은 건너뜁니다. 왼쪽은 점광원 512개의 조명 결과, 오른쪽은 광원별 계산 영역을 색 사각형으로 표시한 화면입니다.

점광원 2,048개에서 Scissor Rect를 쓰지 않으면 GPU 점유율이 100%에 도달해 프레임이 안정적으로 나오지 않았습니다. Scissor Rect를 적용하면 GPU 점유율이 20~30%로 유지되고, VSync 환경에서 240 FPS를 안정적으로 유지합니다.

### ShadowScene

![기둥과 동상이 드리운 그림자](Samples/ShadowScene/images/columns-and-statues.png)

광원 종류별로 다른 섀도 매핑을 사용합니다.

- 점광원: 6방향 깊이를 텍스처 배열에 담는 Omni Shadow Mapping
- 스팟라이트: 단일 섀도 맵
- 방향광: 시야 거리를 4개 구간으로 나눈 Cascaded Shadow Mapping

필터는 Hard, PCF(3×3, 5×5, 7×7), VSM 중에서 선택할 수 있습니다.

### CullingScene

![옥트리의 공간 분할과 탑뷰의 카메라 프러스텀](Samples/CullingScene/images/spatial-partition-04.png)

오브젝트 20만 개에 프러스텀 컬링을 두 방식으로 적용해 비교합니다. 선형 컬링은 모든 오브젝트를 하나씩 프러스텀과 검사하는 전수 조사입니다. 옥트리 컬링은 공간을 계층적으로 나눠 오브젝트를 담고, 노드의 영역이 프러스텀 밖이면 그 노드에 속한 오브젝트를 검사하지 않고 통째로 제외합니다. 상위 노드 하나의 테스트가 실패할 때마다 많은 오브젝트의 검사를 건너뛰므로 검사 횟수가 크게 줄어듭니다.

| 컬링 방식 | 평균 FPS | 프레임 시간 |
| --- | ---: | ---: |
| 컬링 없음 | 71.44 | 14.0 ms |
| 선형 컬링 (전수 조사) | 86.92 | 11.5 ms |
| 옥트리 컬링 | 97.20 | 10.3 ms |

20만 개 기준으로 옥트리 컬링은 선형 컬링보다 프레임당 약 1.2 ms 빨랐습니다. 60 FPS의 프레임 예산이 약 16.7 ms인 것을 생각하면, 컬링 단계 하나에서 예산의 7% 이상을 줄인 큰 최적화입니다.

### SSAOScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/SSAOScene/images/scene.png" alt="SSAO를 적용한 Sponza 장면" width="100%"></td>
      <td align="center"><img src="Samples/SSAOScene/images/ambient-occlusion.png" alt="Sponza에서 AO 값만 표시한 화면" width="100%"></td>
    </tr>
  </tbody>
</table>

깊이 버퍼로 뷰 공간 위치를 복원하고, 노멀 방향 반구에 샘플을 뿌려 주변 지오메트리에 가려지는 비율로 차폐를 구합니다. 노이즈 텍스처로 샘플 커널을 회전시키고, 블러를 거친 결과를 앰비언트 조명에 적용합니다. 왼쪽은 SSAO를 적용한 장면, 오른쪽은 AO 값만 출력한 화면으로, 기둥과 벽이 맞닿는 곳이 어둡게 나타납니다.

### SSRScene

![바닥과 구에 적용한 화면 공간 반사](Samples/SSRScene/images/reflections.png)

반사 방향으로 화면 공간 레이 마칭을 하며 깊이 버퍼와 교차하는 지점의 색을 가져옵니다. 화면 가장자리나 교차 판정이 불확실한 곳은 반사 강도를 줄여 경계가 끊겨 보이지 않게 했습니다.

### ParticleScene

![눈 파티클](Samples/ParticleScene/images/snow.png)

파티클의 위치, 속도, 회전, 수명을 구조화 버퍼에 저장하고 컴퓨트 셰이더로 갱신합니다. 정점 셰이더가 같은 버퍼를 직접 읽기 때문에 CPU로의 readback이 없고, `DrawInstancedIndirect` 한 번으로 전체를 그립니다. 파티클 10만 개에서 약 2,400 FPS입니다.

### MousePickingScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/MousePickingScene/images/object-ids.png" alt="오브젝트마다 다른 색으로 표시한 ID 버퍼" width="100%"></td>
      <td align="center"><img src="Samples/MousePickingScene/images/selection-outline.png" alt="선택한 구의 금색 아웃라인" width="100%"></td>
    </tr>
  </tbody>
</table>

MRT로 색상과 오브젝트 ID(`R32_UINT`)를 동시에 기록합니다. 왼쪽 그림은 ID 버퍼를 오브젝트마다 다른 색으로 시각화한 화면입니다. 클릭한 위치의 ID 한 픽셀만 CPU로 읽어 오브젝트를 선택합니다. 아웃라인은 후처리에서 ID 버퍼를 읽어, 선택된 ID와 다른 ID가 맞닿은 경계 픽셀을 칠해 그립니다(오른쪽).

### 기타 샘플

<table width="100%">
  <tbody>
    <tr>
      <td align="center" width="50%"><img src="Samples/SimpleScene/images/rendering.png" alt="SimpleScene" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/MultiVertexStreamScene/images/vertex-streams.png" alt="MultiVertexStreamScene" width="100%"></td>
    </tr>
    <tr>
      <td>SimpleScene: 모델 로딩, 트랜스폼, 노멀 기반 조명</td>
      <td>MultiVertexStreamScene: 위치와 색상을 별도 정점 버퍼 슬롯으로 전달</td>
    </tr>
    <tr>
      <td align="center" width="50%"><img src="Samples/TextureScene/images/texture-mapping.png" alt="TextureScene" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/StencilScene/images/reflection-and-outline.png" alt="StencilScene" width="100%"></td>
    </tr>
    <tr>
      <td>TextureScene: 알베도, 노멀, AO 텍스처 매핑</td>
      <td>StencilScene: 스텐실 버퍼를 이용한 평면 반사와 아웃라인</td>
    </tr>
    <tr>
      <td align="center" width="50%"><img src="Samples/TerrainScene/images/terrain.png" alt="TerrainScene" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/SkinningScene/images/animation.png" alt="SkinningScene" width="100%"></td>
    </tr>
    <tr>
      <td>TerrainScene: Perlin Noise 하이트맵, 높이·경사에 따른 텍스처 블렌딩</td>
      <td>SkinningScene: 선형 블렌드 스키닝, 대기·걷기·공격 애니메이션 간 전환 블렌딩</td>
    </tr>
  </tbody>
</table>

## 빌드 및 실행

Windows x64, C++20 환경에서 빌드합니다. Visual Studio 2026(플랫폼 도구 집합 `v145`)의 "C++를 사용한 데스크톱 개발" 워크로드가 필요합니다. 외부 라이브러리는 vcpkg의 Visual Studio 통합으로 참조하며, 링크 목록(DirectXTex, DirectXMesh, Assimp, fmt, spdlog, EASTL, efsw, lunasvg, yyjson)은 [`Directory.Build.targets`](Directory.Build.targets)에 있습니다.

1. 저장소를 받습니다.

   ```powershell
   git clone https://github.com/JuggernautSensei/JamEngine.git
   ```

2. `JamEngine.sln`을 열고 `SandboxApp`을 시작 프로젝트로 지정합니다.
3. `Release | x64`로 빌드한 뒤 실행합니다. 명령줄에서는 다음 스크립트를 사용합니다.

   ```powershell
   powershell -NoProfile -File scripts/Build.ps1 -Configuration Release
   ```

실행 후 Scenes 창에서 샘플을 고르고, W, A, S, D, Q, E로 카메라를 움직입니다.
