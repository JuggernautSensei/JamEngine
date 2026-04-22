# SIMD 최적화 변경 내역

날짜: 2026-04-18  
SIMD 라이브러리: Google Highway (`hwy/highway.h`)

---

## 개요

기존에 SIMD가 적용되지 않았거나 스칼라 연산으로 남아 있던 부분을 Highway SIMD로 최적화.  
Vec4·Matrix·일부 Quat 연산은 이미 SIMD화되어 있었으므로, 미적용 구간만 대상으로 함.

---

## 파일별 변경 내역

### `JamEngine/Quaternion.h`

#### 1. `Quat::LengthSq()`

| 항목 | 내용 |
|------|------|
| 변경 전 | 스칼라 4-mul + 3-add (`x*x + y*y + z*z + w*w`) |
| 변경 후 | `hn::Load` → `hn::Mul` → `hn::ReduceSum` |
| 근거 | 4개 float이 연속 메모리에 배치되어 있어 128-bit 벡터 로드 가능. `Vec4::LengthSq`와 동일 패턴. |

```cpp
// Before
return x * x + y * y + z * z + w * w;

// After
using D      = hn::ScalableTag<float>;
const auto v = hn::Load(D(), &x);
return hn::ReduceSum(D(), hn::Mul(v, v));
```

---

#### 2. `Quat::Normalize()` / `Quat::Normalized()`

| 항목 | 내용 |
|------|------|
| 변경 전 | `sqrt` → 4회 나눗셈 (division은 mul보다 4~10배 느림) |
| 변경 후 | `InvSqrt` (SSE `rsqrt`) → SIMD Mul 1회 |
| 근거 | `Vec4::Normalize`와 동일 패턴. `rsqrt`는 12-bit 정밀도이나 쿼터니언 정규화에 충분. 나눗셈 4회 → SIMD 곱셈 1회로 처리량 향상. |

```cpp
// Before
const float len = Length();
x /= len; y /= len; z /= len; w /= len;

// After
const float invLen = Jam::InvSqrt(LengthSq());   // SSE rsqrt
const auto  v      = hn::Load(D(), &x);
const auto  s      = hn::Set(D(), invLen);
hn::Store(hn::Mul(v, s), D(), &x);
```

---

#### 3. `Quat::operator-()` (단항 부호 반전)

| 항목 | 내용 |
|------|------|
| 변경 전 | 스칼라 4-neg (`{ -x, -y, -z, -w }`) |
| 변경 후 | `hn::Load` → `hn::Neg` → `hn::Store` |
| 근거 | `Vec4::operator-`와 동일 패턴. SIMD Neg는 XOR sign-bit 1회. |

```cpp
// Before
return { -x, -y, -z, -w };

// After
const auto v = hn::Load(D(), &x);
Quat ret     = {};
hn::Store(hn::Neg(v), D(), &ret.x);
return ret;
```

---

### `JamEngine/BoundingVolume.cpp`

#### 4. `(anonymous)::IntersectPlaneAabb_()`

가장 핫한 경로: `Frustum::Intersects(Aabb)`에서 6회 호출되며, 한 프레임에 수백~수천 번 실행됨.

| 항목 | 내용 |
|------|------|
| 변경 전 | `Vec3::Dot` (스칼라 3-mul + 2-add) 2회 + `Vec3::Abs` (스칼라 3-abs) |
| 변경 후 | `Vec4` 래핑 (w=0) 후 `Vec4::Dot` (SIMD 4-lane) + `Vec4::Abs` (SIMD Neg) |
| 근거 | Vec3은 3-float이라 직접 SIMD load가 불안전. Vec4{xyz, 0}으로 감싸면 w=0끼리 dot product에서 0 기여 → 정확도 동일. 4-float SIMD로 2개의 내적을 각 1 SIMD Mul + ReduceSum으로 처리. |

```cpp
// Before
const float dist = _normal.Dot(_center) + _d;   // 스칼라 Vec3 dot
const float r    = _normal.Abs().Dot(_ext);      // 스칼라 Vec3 abs + dot

// After
const Vec4  n4   { _normal.x, _normal.y, _normal.z, 0.f };
const Vec4  c4   { _center.x, _center.y, _center.z, 0.f };
const Vec4  e4   { _ext.x,    _ext.y,    _ext.z,    0.f };
const float dist = n4.Dot(c4) + _d;   // SIMD Vec4 dot (w=0 기여 없음)
const float r    = n4.Abs().Dot(e4);  // SIMD Vec4 abs + dot
```

---

#### 5. `Aabb::Transform(const Matrix&)` — 확장 벡터 재계산

| 항목 | 내용 |
|------|------|
| 변경 전 | `Vec3{...}.Abs()` → `Vec3::Dot(e)` × 3회 (스칼라 9-mul + 6-add + 9-abs) |
| 변경 후 | `Vec4{..., 0}.Abs()` → `Vec4::Dot(e4)` × 3회 (SIMD Abs + SIMD ReduceSum × 3) |
| 근거 | 회전 행렬의 각 행 3개 요소를 Vec4(w=0)로 올려 SIMD Abs와 Dot을 활용. 스칼라 Abs×9 → SIMD Abs(4-wide)×3. 특히 스키닝/파티클 시스템처럼 많은 수의 Aabb를 매 프레임 Transform할 때 효과적. |

```cpp
// Before
const Vec3 r0   = Vec3 { _mtx.m[0][0], _mtx.m[0][1], _mtx.m[0][2] }.Abs();
const Vec3 r1   = Vec3 { _mtx.m[1][0], _mtx.m[1][1], _mtx.m[1][2] }.Abs();
const Vec3 r2   = Vec3 { _mtx.m[2][0], _mtx.m[2][1], _mtx.m[2][2] }.Abs();
const Vec3 newE = { r0.Dot(e), r1.Dot(e), r2.Dot(e) };

// After
const Vec4 e4  { e.x, e.y, e.z, 0.f };
const Vec4 ar0 = Vec4 { _mtx.m[0][0], _mtx.m[0][1], _mtx.m[0][2], 0.f }.Abs();
const Vec4 ar1 = Vec4 { _mtx.m[1][0], _mtx.m[1][1], _mtx.m[1][2], 0.f }.Abs();
const Vec4 ar2 = Vec4 { _mtx.m[2][0], _mtx.m[2][1], _mtx.m[2][2], 0.f }.Abs();
const Vec3 newE { ar0.Dot(e4), ar1.Dot(e4), ar2.Dot(e4) };
```

---

## 미적용 항목 및 이유

| 대상 | 미적용 이유 |
|------|-------------|
| `Vec3` 전체 | 3-float 구조체 (12 bytes). 4-float SIMD load 시 인접 메모리 읽기 → UB 위험. 패딩 추가 시 GPU 버퍼 레이아웃 깨짐. |
| `Aabb::Contains(Vec3/Aabb)` | 분리 조건 발생 시 early-out하는 스칼라 분기가 더 유리. SIMD 분기 제거는 오히려 평균적으로 더 느릴 수 있음. |
| `Aabb::Intersects(Aabb)` | 동일 이유. SAT early-out 패턴은 스칼라가 최적. |
| `Matrix::operator*(Matrix)` | 기존 구현의 column-access 패턴이 엔진 고유 레이아웃 규약에 의존. 파악 없이 변경 시 렌더링 오류 위험. |
| `Plane::Normalize` / `Frustum::Normalize` | Vec3 normal (3-float) SIMD 미지원과 동일 이유. 호출 빈도도 낮음. |
| `Math.h` `Sqrt`/`InvSqrt` | 이미 `_mm_sqrt_ss` / `_mm_rsqrt_ss` SSE 사용 중. |

---

---

## Transform.cpp — math 호환 재작성 (2026-04-18 추가)

### 호환 불일치 목록 및 수정 내용

| 구분 | 기존 코드 | 수정 후 |
|------|-----------|---------|
| 행렬 규약 | `_ij` 접근 + 이동 bottom row | `m[i][j]` + Matrix.h SRT 규약 (이동 w-column) |
| `GetMatrix()` | 직접 `_ij` 설정 (R^T 오기입) | `Matrix::CreateSRT(s, R, t)` 위임 |
| `CalcInverseMatrix()` | 스칼라, 이동 bottom row | SRT 역행렬 수식 + Vec4 SIMD Dot (w=0 패턴) |
| `SetMatrix()` | `_41/_42/_43` = 이동 | `m[0..2][3]` = 이동, 스케일 from row norm |
| `Vec3::Transform(v, q)` | 존재하지 않는 static | `q.RotateVector(v)` |
| `Vec3::kRight/kUp/kForward` | 없음 | `Vec3::kUnitX/Y/Z` |
| `Quat::CreateFromAxisAngle` | 없음 | Quaternion.h에 static 추가 (생성자 래핑) |
| `Quat::CreateFromYawPitchRoll` | 없음 | Quaternion.h에 static 추가 (SetEuler 래핑) |
| `Quat::CreateFromRotationMatrix` | 없음 | Quaternion.cpp에 Shepperd's method 구현 |
| `m_rotation.Conjugate()` | 반환값 미할당 (버그) | `m_rotation = m_rotation.Conjugate()` |
| `Normalized(q)` 자유 함수 | 없음 | Quaternion.h에 inline 추가 |
| `Cross/Normalized/Length/Dot/AsVec4` 자유 함수 | 없음 | Vector.h에 Vec3 thin wrapper 추가 |

### CalcInverseMatrix SIMD 최적화

```
M^-1[i][j]   = R[j][i] * invS[j]          (3×3 부분)
M^-1[i][3]   = -dot(M^-1_row_i, t)        (이동 열)
```

변환 열 3개 내적을 `Vec4(w=0).Dot()` SIMD로 처리:
```cpp
const Vec4 irow0 { r00*invSx, r10*invSy, r20*invSz, 0.f };
const Vec4 t4    { tx, ty, tz, 0.f };
// -irow0.Dot(t4) → 4-lane SIMD mul + ReduceSum
```

### 추가된 파일

| 파일 | 추가 내용 |
|------|-----------|
| `Vector.h` | `Dot, Cross, Length, Normalized, AsVec4` (Vec3 자유 함수) |
| `Quaternion.h` | `CreateFromAxisAngle, CreateFromYawPitchRoll` (inline static), `CreateFromRotationMatrix` (선언), `Normalized` (자유 함수), `struct Matrix` forward declaration |
| `Quaternion.cpp` | `Quat::CreateFromRotationMatrix` — Shepperd's method 구현 |
| `Transform.cpp` | 전면 재작성 (math 호환 + SIMD CalcInverseMatrix) |

---

## 기존 SIMD 적용 현황 (변경 없음)

| 대상 | 적용 내용 |
|------|-----------|
| `Vec4` 전체 | Add/Sub/Mul/Div, Min/Max/Clamp, Dot, LengthSq, Normalize, Lerp, Reflect, Refract, Abs, Neg |
| `Matrix` 전체 | Add/Sub/Mul, Transpose (InterleaveLower/Upper), Inverse (SIMD MulScalar), Vec3/Vec4 * Matrix |
| `Quat` 일부 | Dot, Lerp, Slerp, operator*scalar, operator/scalar, operator+, operator- |

---

## 성능 예상 효과

| 함수 | 예상 개선 |
|------|-----------|
| `Quat::LengthSq` | ~20–30% (SIMD 병렬 mul → horizontal sum) |
| `Quat::Normalize/Normalized` | ~40–50% (rsqrt + SIMD mul vs sqrt + 4 div) |
| `IntersectPlaneAabb_` | ~15–25% (SIMD Abs+Dot vs 스칼라 Vec3 연산) × 6 planes |
| `Aabb::Transform` | ~20–30% (SIMD Abs+Dot×3 vs 스칼라 Vec3×3) |

> 실제 수치는 CPU 아키텍처, 컴파일러 자동 벡터화 여부, 캐시 상태에 따라 달라짐.  
> JamBenchmark 프로젝트에서 `OctreeBenchmark` 등으로 실측 권장.
