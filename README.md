<h1 align="center">JamEngine</h1>

<p align="center">D3D11 기반 C++20 게임 개발 프레임워크</p>

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

부경대학교 캡스톤디자인 경진대회 출품을 위해 9개월간 개발한 D3D11 기반 게임 엔진 프레임워크입니다. 에디터에서 모델을 불러와 씬을 구성하고, 엔티티에 컴포넌트와 스크립트를 붙여 실행할 수 있습니다. 엔진은 정적 라이브러리 `JamEngine`이고, 이를 사용하는 `SandboxApp`에 그래픽스 기법을 하나씩 구현한 샘플 씬들이 있습니다.

<table width="100%">
  <thead>
    <tr>
      <th align="left">모듈</th>
      <th align="left">역할</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><a href="JamEngine/Application.h"><code>Application</code></a>, <a href="JamEngine/SceneManager.h"><code>SceneManager</code></a></td>
      <td>창과 하위 시스템 초기화, 메인 루프, 씬 전환</td>
    </tr>
    <tr>
      <td><a href="JamEngine/Scene.h"><code>Scene</code></a>, <a href="JamEngine/Entity.h"><code>Entity</code></a></td>
      <td>EnTT 기반 엔티티·컴포넌트, 부모·자식 씬 그래프와 로컬·월드 변환</td>
    </tr>
    <tr>
      <td><a href="JamEngine/AssetManager.h"><code>AssetManager</code></a></td>
      <td>모델, 메시, 재질, 텍스처를 경로로 관리하고 모델의 노드 구조로 엔티티 생성</td>
    </tr>
    <tr>
      <td><a href="JamEngine/Graphics.h"><code>Graphics</code></a></td>
      <td>D3D11 버퍼, 텍스처, 셰이더, 렌더 상태를 핸들로 관리하고 드로우·디스패치 제출</td>
    </tr>
    <tr>
      <td><a href="JamEngine/RenderGraph.h"><code>RenderGraph</code></a></td>
      <td>출력에 필요한 패스와 부수 효과가 있는 패스를 선별하고 자원 바인딩·해제 명령을 생성</td>
    </tr>
  </tbody>
</table>

이 밖에 수학(벡터, 행렬, 사원수), 바운딩 볼륨과 옥트리, 파일 감시, JSON 아카이브, 메모리 풀 같은 기반 모듈이 엔진에 포함되어 있습니다. `JamTest`에 단위 테스트가, `JamBenchmark`에 수학·문자열·옥트리 등의 벤치마크가 있습니다.

## 에디터

![엔티티를 만들고 기즈모로 옮긴 뒤 인스펙터에서 렌더러블 컴포넌트를 추가하고 색과 크기를 바꿔 복제하는 장면](Samples/Editor/images/entity-editing.gif)

ImGui로 만든 에디터는 씬 계층, 씬 뷰, 엔티티 인스펙터, 파일 브라우저, 프로파일러 창으로 구성됩니다. 씬 계층에서 엔티티를 만들고 복제하고 지우며, 부모·자식 구조를 트리로 봅니다. 씬 뷰에서는 기즈모로 이동, 회전, 크기를 월드 또는 로컬 기준으로 조작합니다. 인스펙터의 컴포넌트 추가 목록과 컴포넌트별 편집 패널은 아래 [컴포넌트 리플렉션](#컴포넌트-리플렉션)의 표에서 만들어지므로, 새 컴포넌트를 등록하면 에디터 코드를 고치지 않아도 인스펙터에 나타납니다.

![파일 브라우저에서 FBX 모델을 임포트해 씬에 배치하고, 텍스처를 재질 슬롯에 지정하는 장면](Samples/Editor/images/model-import.gif)

파일 브라우저에서 FBX 같은 외부 모델과 이미지를 임포트하면 엔진 형식의 메시(`.jmsh`), 재질(`.jmat`), 모델(`.jmdl`) 파일이 만들어집니다. 모델을 씬에 놓으면 노드 구조대로 엔티티 계층이 생기고, 인스펙터에서 알베도, 노멀 등 재질 슬롯에 텍스처를 지정하면 바로 반영됩니다.

![Sponza 씬을 파일로 저장하고 빈 씬에서 다시 불러온 뒤, 카메라 컴포넌트를 붙여 게임 화면으로 전환하는 장면](Samples/Editor/images/scene-serialization.gif)

씬은 파일로 저장하고 다시 불러올 수 있습니다. 각 컴포넌트의 `Serialize`/`Deserialize`가 자기 데이터를 기록하고 복원하므로, 씬 저장 코드는 컴포넌트 종류를 모릅니다. 상단의 run 버튼을 누르면 에디터 카메라 대신 카메라 컴포넌트를 가진 엔티티의 시점으로 게임 화면이 실행되고, edit 버튼으로 편집 화면으로 돌아옵니다.

![스크립트로 움직이는 큐브를 게임 화면과 편집 화면을 오가며 확인하는 장면](Samples/Editor/images/script-play.gif)

게임 로직은 `OnStart`, `OnUpdate`, `OnStop`을 재정의한 스크립트 클래스로 작성해 스크립트 컴포넌트로 엔티티에 붙입니다. 위 장면은 방향키 입력으로 소유 엔티티의 위치를 옮기는 스크립트를 실행한 결과입니다.

## 컴포넌트 리플렉션

엔진이 다루는 컴포넌트는 위치, 카메라, 조명, 렌더러블처럼 종류가 계속 늘어납니다. 엔티티를 복제하거나, 씬을 저장하거나, 에디터에서 속성을 편집하는 코드가 컴포넌트 타입을 하나하나 알고 있으면, 컴포넌트를 추가할 때마다 그 코드를 모두 고쳐야 합니다. 그래서 컴포넌트가 자기 능력을 선언하면 엔진이 시작 시점에 타입별 함수 표를 만들고, 엔진 코드는 이 표만 보고 컴포넌트를 다루게 했습니다.

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

`JAM_COMPONENT_BODY`는 컴포넌트 안에 `inline static` 등록 객체를 하나 둡니다. 이 객체의 생성자가 프로그램 시작 시 [`ComponentDatabase`](JamEngine/ComponentDatabase.h)에 타입을 등록하므로, 컴포넌트를 추가할 때 등록 코드를 따로 쓰지 않습니다. 등록할 때는 생성, 복사 생성, 제거, 조회, 존재 확인 함수를 템플릿으로 만들어 `Delegate`로 저장하고, 선언한 플래그에 따라 직렬화와 편집 패널 함수도 함께 저장합니다.

플래그와 실제 구현이 어긋나면 컴파일 오류가 납니다. `Serializable`을 선언했는데 `Serialize`/`Deserialize`가 없거나, `Editable`을 선언했는데 `DrawEditPanel`이 없으면 등록 시점의 `static_assert`가 컴포넌트 이름과 함께 실패합니다.

```cpp
static_assert(!HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Serializable> || CheckComponentSerializablePropertyValidation<TCmp>,
              "Component '" #TCmp "': Serializable 플래그에 맞는 시그니처가 없습니다");
```

엔진 코드는 이 표를 순회하며 타입을 모른 채 컴포넌트를 다룹니다. 예를 들어 엔티티 복제는 등록된 모든 컴포넌트 중 원본이 가진 것을 찾아 복사 생성 함수를 부르고, 자식 엔티티까지 재귀적으로 복제합니다. 새 컴포넌트를 만들어도 복제 코드는 바뀌지 않습니다.

```cpp
for (const ComponentDatabase::Data& data: db.GetComponentDataContainer() | std::views::values)
{
    if (data.Has(_srcEntity))
    {
        data.CreateByCopy(entity, data.Get(_srcEntity));
    }
}
```

## 렌더 그래프

디퍼드 조명이나 SSAO처럼 여러 패스가 앞 패스의 결과를 읽는 렌더링에서는, 패스마다 렌더 타깃과 텍스처를 바인딩하고 해제하는 코드가 길어집니다. D3D11은 같은 텍스처를 렌더 타깃과 셰이더 입력으로 동시에 바인딩할 수 없으므로, 이전 패스에서 쓴 텍스처를 읽기 전에 렌더 타깃에서 풀어야 하고, 반대로 읽던 텍스처에 쓰기 전에는 셰이더 입력에서 풀어야 합니다. 패스를 추가하거나 순서를 바꿀 때마다 이 해제 코드를 다시 맞추는 것은 실수가 생기기 쉬웠습니다.

[`RenderGraph`](JamEngine/RenderGraph.h)에서는 각 패스가 무엇을 읽고 어디에 쓰는지만 이름으로 선언합니다. 아래는 SSAO 샘플의 일부입니다.

```cpp
m_graph.AddRenderPass("Pass_Ssao")
    .SetFrameBuffer("FB_Ssao")
    .SetProgram("P_Ssao")
    .SetTexture("T_GNormal", eShader::PixelShader, 0)
    .SetTexture("T_GDepth", eShader::PixelShader, 1)
    .SetConstantBuffer("CB_Ssao", eShader::PixelShader, CBSsao::kSlot)
    .SetExecutor(Bind<&SSAOScene::SubmitSsao_>(this));
```

그래프는 처음 실행될 때와 패스 구성이 바뀔 때 컴파일되며, 두 단계를 거칩니다.

먼저 쓰이지 않는 패스를 제거합니다. 자원 이름으로 "이 패스가 읽는 자원을 어느 패스가 쓰는가"를 따라 역방향 간선을 만들고, 화면에 출력하는 최종 패스와 부수 효과가 있다고 표시한 패스에서 역방향으로 탐색합니다. 탐색에 닿지 않은 패스는 결과가 아무 데도 쓰이지 않으므로 실행 목록에서 빠집니다. 예를 들어 결과를 화면에 내보내지 않는 시각화 패스를 꺼 두면, 그 패스만 쓰던 앞 단계 패스도 함께 실행 목록에서 빠집니다.

다음으로 남은 패스를 순서대로 따라가며 슬롯마다 현재 어떤 자원이 어떤 용도로 바인딩되어 있는지 가상으로 추적합니다. 읽으려는 자원이 렌더 타깃이나 쓰기 가능 버퍼로 묶여 있으면 그 바인딩을 먼저 해제하는 명령을, 쓰려는 자원이 셰이더 입력으로 묶여 있으면 그 슬롯을 해제하는 명령을 패스에 기록합니다. 같은 자원이 이미 같은 슬롯에 읽기용으로 묶여 있으면 바인딩 명령을 생략합니다. 마지막 패스의 바인딩 상태가 다음 프레임의 첫 패스까지 이어지므로 이 추적을 두 바퀴 돌리고, 두 번째 바퀴의 해제 목록을 사용합니다.

실행 시에는 컴파일 결과의 명령만 순서대로 적용하므로 매 프레임 의존성을 다시 계산하지 않습니다. 창 크기가 바뀌면 크기를 따라가도록 지정한 텍스처와 프레임 버퍼만 다시 만들고, 패스 구조는 그대로 둔 채 핸들 값만 갱신합니다.

## 렌더링 샘플

SandboxApp의 Scenes 창에서 샘플을 고를 수 있습니다. 모든 샘플은 같은 `Scene` 인터페이스를 구현하고, 여러 패스가 필요한 샘플은 렌더 그래프로 구성했습니다.

### InstancingScene

![65,536개 큐브의 인스턴싱 렌더링](Samples/InstancingScene/images/cubes.png)

같은 큐브 65,536개를 그립니다. 큐브마다 변환 행렬을 갱신하고 드로우를 호출하면 190–210 FPS, 변환 행렬을 인스턴스 버퍼에 모아 드로우 한 번으로 그리면 550–600 FPS입니다. 범위의 중간값으로 프레임 시간을 비교하면 5.0 ms → 1.7 ms(약 2.9배)입니다.

### PBRScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/PBRScene/images/statue-bright-environment.png" alt="밝은 환경에서의 동상 반사" width="100%"></td>
      <td align="center"><img src="Samples/PBRScene/images/statue-warm-environment.png" alt="따뜻한 환경에서의 동상 반사" width="100%"></td>
    </tr>
  </tbody>
</table>

Metallic과 Roughness를 쓰는 물리 기반 렌더링에 IBL을 더했습니다. 확산광은 Irradiance 맵으로, 정반사는 Roughness별로 필터링한 환경 맵과 BRDF LUT로 근사하므로, 환경을 바꾸면 같은 동상에 비치는 색과 밝기가 달라집니다.

### DeferredScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/DeferredScene/images/point-lights-512.png" alt="512개 점광원의 조명 결과" width="100%"></td>
      <td align="center"><img src="Samples/DeferredScene/images/scissor-rects.png" alt="점광원마다 처리 영역을 색 사각형으로 표시한 Scissor Rect" width="100%"></td>
    </tr>
  </tbody>
</table>

색상, 노멀, 깊이를 G-buffer에 기록한 뒤 광원마다 조명을 더합니다. 점광원은 영향 반경이 정해져 있으므로, 반경을 감싸는 정육면체의 여덟 꼭짓점을 화면에 투영해 얻은 사각형을 Scissor Rect로 지정하고 그 안의 픽셀만 조명을 계산합니다. 사각형이 화면 밖이면 그 광원은 그리지 않습니다. 오른쪽 그림의 색 사각형이 광원마다 계산하는 영역입니다. 점광원 2,048개에서 약 240 FPS입니다.

### ShadowScene

![기둥과 동상이 드리운 그림자](Samples/ShadowScene/images/columns-and-statues.png)

광원 종류마다 섀도 맵 방식이 다릅니다. 점광원은 여섯 방향의 깊이를 텍스처 배열에 담는 Omni Shadow Mapping, 스팟라이트는 한 장의 섀도 맵, 방향광은 가시 거리를 네 구간으로 나눠 가까운 곳에 해상도를 더 주는 Cascaded Shadow Mapping을 씁니다. 필터는 Hard, PCF(3×3, 5×5, 7×7), VSM 중에서 고를 수 있습니다.

### CullingScene

![옥트리의 공간 분할과 탑뷰의 카메라 프러스텀](Samples/CullingScene/images/spatial-partition-04.png)

오브젝트 20만 개를 옥트리에 나눠 담고, 노드의 영역이 프러스텀 밖이면 그 안의 오브젝트를 한꺼번에 제외합니다.

<table width="100%">
  <thead>
    <tr>
      <th align="left">컬링 방식</th>
      <th align="right">평균 FPS</th>
      <th align="right">프레임 시간</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>컬링 없음</td>
      <td align="right">71.44</td>
      <td align="right">14.0 ms</td>
    </tr>
    <tr>
      <td>오브젝트마다 선형 검사</td>
      <td align="right">86.92</td>
      <td align="right">11.5 ms</td>
    </tr>
    <tr>
      <td>옥트리</td>
      <td align="right">97.20</td>
      <td align="right">10.3 ms</td>
    </tr>
  </tbody>
</table>

선형 검사는 20만 개를 모두 프러스텀과 비교하지만, 옥트리는 바깥 노드를 통째로 건너뛰어 검사 횟수가 줄어듭니다. 선형 검사 대비 11.5 ms → 10.3 ms, 컬링이 없을 때 대비 14.0 ms → 10.3 ms(약 1.36배)입니다.

### SSAOScene · SSRScene

<table width="100%">
  <tbody>
    <tr>
      <td align="center"><img src="Samples/SSAOScene/images/ambient-occlusion.png" alt="Sponza의 AO만 표시한 화면" width="100%"></td>
      <td align="center"><img src="Samples/SSRScene/images/reflections.png" alt="바닥과 구에 적용한 화면 공간 반사" width="100%"></td>
    </tr>
  </tbody>
</table>

SSAO는 깊이로 뷰 공간 위치를 복원하고, 노멀 방향 반구에 놓은 샘플이 주변 깊이에 가려지는 비율로 차폐를 구합니다. 노이즈 텍스처로 샘플 방향을 돌리고 블러로 정리한 뒤 주변광에만 적용합니다. SSR은 반사 표면에서 레이를 진행시켜 깊이 버퍼와 만나는 위치의 색을 가져오고, 화면 가장자리나 교차가 불확실한 곳에서는 반사를 약하게 합니다.

### ParticleScene

![눈 파티클](Samples/ParticleScene/images/snow.png)

파티클의 위치, 속도, 회전, 수명을 구조화 버퍼에 두고 컴퓨트 셰이더로 갱신합니다. 정점 셰이더가 같은 버퍼를 읽어 그리므로 데이터가 CPU로 돌아오지 않고, 전체를 `DrawInstancedIndirect` 한 번으로 그립니다. 파티클 10만 개에서 약 2,400 FPS입니다.

### MousePickingScene

![선택한 구의 금색 아웃라인](Samples/MousePickingScene/images/selection-outline.png)

색상과 오브젝트 ID(`R32_UINT`)를 서로 다른 렌더 타깃에 동시에 기록하고, 클릭한 위치의 ID 한 픽셀만 CPU로 읽어 선택합니다. 아웃라인은 후처리에서 ID 버퍼를 읽어, 선택된 ID 중 주변에 다른 ID가 있는 픽셀을 칠합니다.

### 기타 샘플

<table width="100%">
  <tbody>
    <tr>
      <td align="center" width="50%"><img src="Samples/SimpleScene/images/rendering.png" alt="SimpleScene: 모델 로딩, 변환, 노멀 기반 조명" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/MultiVertexStreamScene/images/vertex-streams.png" alt="MultiVertexStreamScene: 위치와 색상을 서로 다른 정점 버퍼 슬롯으로 전달" width="100%"></td>
    </tr>
    <tr>
      <td>SimpleScene: 모델 로딩, 변환, 노멀 기반 조명</td>
      <td>MultiVertexStreamScene: 위치와 색상을 서로 다른 정점 버퍼 슬롯으로 전달</td>
    </tr>
    <tr>
      <td align="center" width="50%"><img src="Samples/TextureScene/images/texture-mapping.png" alt="TextureScene: 알베도, 노멀, AO 텍스처 매핑" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/StencilScene/images/reflection-and-outline.png" alt="StencilScene: 스텐실로 제한한 평면 반사와 아웃라인" width="100%"></td>
    </tr>
    <tr>
      <td>TextureScene: 알베도, 노멀, AO 텍스처 매핑</td>
      <td>StencilScene: 스텐실로 제한한 평면 반사와 아웃라인</td>
    </tr>
    <tr>
      <td align="center" width="50%"><img src="Samples/TerrainScene/images/terrain.png" alt="TerrainScene: Perlin Noise 높이맵, 높이·경사별 텍스처 혼합" width="100%"></td>
      <td align="center" width="50%"><img src="Samples/SkinningScene/images/animation.png" alt="SkinningScene: 선형 블렌딩 스키닝, 애니메이션 전환 블렌딩" width="100%"></td>
    </tr>
    <tr>
      <td>TerrainScene: Perlin Noise 높이맵, 높이·경사별 텍스처 혼합</td>
      <td>SkinningScene: 선형 블렌딩 스키닝, 애니메이션 전환 블렌딩</td>
    </tr>
  </tbody>
</table>

## 빌드 및 실행

Windows x64에서 C++20으로 빌드합니다. Visual Studio 2026(플랫폼 도구 집합 `v145`)의 "C++를 사용한 데스크톱 개발" 워크로드가 필요하고, entt, fmt, DirectXTex, unordered_dense는 vcpkg로 설치됩니다.

1. 저장소를 받습니다.

   ```powershell
   git clone https://github.com/JuggernautSensei/JamEngine.git
   ```

2. `JamEngine.sln`을 Visual Studio로 열고 `SandboxApp`을 시작 프로젝트로 지정합니다.
3. 구성을 `x64`로 두고 빌드한 뒤 실행합니다. 명령줄에서는 `scripts/Build.ps1`을 사용합니다.

   ```powershell
   powershell -NoProfile -File scripts/Build.ps1 -Configuration Release
   ```

실행하면 ImGui의 Scenes 창에서 샘플을 고를 수 있고, 카메라는 W, A, S, D, Q, E로 움직입니다.
