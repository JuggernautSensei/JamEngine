# 외부 에셋 출처

외부 모델·텍스처·환경맵의 출처와 라이선스입니다.

| 파일 | 출처 | 라이선스 |
|---|---|---|
| `TerrainScene/Grass.jpg` | ambientCG `Grass001` (1K-JPG, Color) | CC0 |
| `TerrainScene/Rock.jpg` | ambientCG `Rock058` (1K-JPG, Color) | CC0 |
| `TerrainScene/Snow.jpg` | ambientCG `Snow015` (1K-JPG, Color) | CC0 |
| `TerrainScene/Dirt.jpg` | ambientCG `Ground037` (1K-JPG, Color) | CC0 |
| `SSAOScene/Sponza.glb` | Khronos glTF-Sample-Assets `Models/Sponza/glTF`를 외부 이미지 파일까지 하나의 `.glb`로 묶은 것. 엔진 임포터가 내장 텍스처만 읽기 때문. 내용은 변경 없음 (원본 Crytek / Frank Meinl, Marko Dabrovic, Morgan McGuire 수정본, PBR 텍스처 Alexandre Pestana) | © 2016 Crytek, CRYENGINE Limited License Agreement. CC0가 아님. 저장소에 공개하기 전에 재배포 조건을 확인해야 함 |
| `SkinningScene/Paladin_Walk.fbx` | Mixamo `Paladin J Nordstrom` 캐릭터 + `Unarmed Walk Forward` 애니메이션 (With Skin) | Adobe Mixamo 이용 약관. 저장소에 공개하기 전에 재배포 조건을 확인해야 함 |
| `SkinningScene/Idle.fbx`, `Walk.fbx`, `Slash.fbx` | Mixamo "Sword and Shield Pack" (`sword and shield idle`, `walk`, `slash`) | Adobe Mixamo 이용 약관. 저장소에 공개하기 전에 재배포 조건을 확인해야 함 |
| `TextureScene/Materials/cobblestone_floor_01_*.jpg`, `SkinningScene/Floor_Albedo.jpg` | Poly Haven [Cobblestone Floor 01](https://polyhaven.com/a/cobblestone_floor_01), 1K JPG. 군중 바닥은 같은 Diffuse 이미지의 복사본 | CC0 |
| `TextureScene/Materials/castle_brick_07_*.jpg` | Poly Haven [Castle Brick 07](https://polyhaven.com/a/castle_brick_07), 1K JPG | CC0 |
| `TextureScene/Materials/brown_planks_03_*.jpg`, `PBRScene/Materials/brown_planks_03_*.jpg` | Poly Haven [Brown Planks 03](https://polyhaven.com/a/brown_planks_03), 1K JPG | CC0 |
| `TextureScene/Materials/aerial_rocks_02_*.jpg` | Poly Haven [Aerial Rocks 02](https://polyhaven.com/a/aerial_rocks_02), 1K JPG | CC0 |
| `TextureScene/Materials/blue_metal_plate_*.jpg` | Poly Haven [Blue Metal Plate](https://polyhaven.com/a/blue_metal_plate), 1K JPG | CC0 |
| `TextureScene/Materials/brown_leather_*.jpg` | Poly Haven [Brown Leather](https://polyhaven.com/a/brown_leather), 1K JPG | CC0 |
| `TextureScene/Materials/denim_fabric_*.jpg` | Poly Haven [Denim Fabric](https://polyhaven.com/a/denim_fabric), 1K JPG | CC0 |
| `Models/Gallery/MarbleBust/*` | Poly Haven [Marble Bust 01](https://polyhaven.com/a/marble_bust_01), Rico Cilliers. 1K glTF와 재질 | CC0 |
| `Models/Gallery/GothicStatue/*` | Poly Haven [Gothic Statue](https://polyhaven.com/a/gothic_statue), Benny Weimer. 1K glTF와 재질 | CC0 |
| `Models/Gallery/BronzeShark/*` | Poly Haven [Bronze Shark Statue](https://polyhaven.com/a/bronze_shark_statue), Tina. 1K glTF와 재질 | CC0 |
| `Models/Gallery/BrassVase/*` | Poly Haven [Brass Vase 03](https://polyhaven.com/a/brass_vase_03). 1K glTF와 재질 | CC0 |
| `Models/Gallery/Column/*` | Quaternius [Column](https://poly.pizza/m/wLubNpOTX4), Poly Pizza에서 GLB 다운로드 | CC0 |
| `Models/Gallery/Column/{Albedo,NormalDX,NormalGL,Arm,Ao}.jpg` | Poly Haven [Marble 01](https://polyhaven.com/a/marble_01), Rob Tuytel. 기둥용 1K 재질 | CC0 |
| `PBRScene/Environments/Studio/*` | Poly Haven [Studio Small 03](https://polyhaven.com/a/studio_small_03), 1K Radiance HDR에서 생성 | CC0 |
| `PBRScene/Environments/Courtyard/*` | Poly Haven [Courtyard](https://polyhaven.com/a/courtyard), 1K Radiance HDR에서 생성 | CC0 |

재질 파일의 접미사: `_Albedo`는 Diffuse, `_Ao`는 AO, `_Arm`은 R=AO/G=Roughness/B=Metallic입니다. `_Normal`은 TextureScene에서 DirectX normal (`nor_dx`), PBR에서 OpenGL normal (`nor_gl`)을 사용합니다. Poly Haven 에셋은 [CC0 라이선스](https://polyhaven.com/license)입니다.

Gallery 모델은 노드 변환을 적용하고 프리미티브를 합친 뒤 바닥 Y=0, 높이 1로 정규화했습니다. 기둥에는 반복 가능한 UV를 적용했습니다.

Studio·Courtyard 환경은 RGBA16F DDS 큐브맵입니다. `Environment.dds`는 면당 256px, `Diffuse.dds`는 32px의 irradiance/π, `Specular.dds`는 128px·8밉의 GGX 프리필터입니다.

ambientCG: https://ambientcg.com
Khronos glTF-Sample-Assets: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Sponza
