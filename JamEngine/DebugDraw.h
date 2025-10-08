#pragma once

/* clang-format off */
namespace jam { struct RGBA; class Mesh; enum class eDetailLevel; }
/* clang-format on */

/// @brief 시각화용 디버그 드로우 시스템입니다.
/// @note Init() ~ Shutdown() 을 호출하여 시스템을 초기화 및 정리할 수 있습니다.
/// Begin() ~ End() 사이에서 드로우 콜을 기록할 수 있습니다.
/// Begin() End()를 호출하지 않고 드로우 콜을 호출하면 예기치 못한 동작이 발생할 수 있습니다.
/// Setxxx 을 호출하여 변환 및 렌더링 상태를 설정할 수 있습니다.
/// Pushxxx ~ Popxxx 쌍을 사용하여 상태를 저장하고 복원할 수 있습니다.
namespace jam
{
namespace debug_draw
{

    /// @brief 디버그 드로우 시스템을 초기화합니다.
    void Init();

    /// @brief 디버그 드로우 시스템을 종료하고 리소스를 정리합니다.
    void Shutdown();

    /// @brief 디버그 드로우를 시작합니다.
    /// @note 내부 상태를 초기화하고 드로우 콜을 기록할 준비를 합니다.
    void Begin();

    /// @brief 디버그 드로우를 종료합니다.
    /// @note 기록된 드로우 콜을 GPU에 제출하고, 내부 상태를 정리합니다.
    void End();

    /////////////////////////////////////////
    ///          TRANSFORM STATE          ///
    /////////////////////////////////////////

    /// @brief 변환 스택에 현재 변환을 저장합니다.
    /// @note 이후의 도형은 해당 변환의 영향을 받고, 변환은 누적됩니다.
    void PushTransform();

    /// @brief 변환 스택에서 이전 변환을 복원합니다.
    void PopTransform();

    /// @brief 카메라 위치와 뷰-프로젝션 행렬을 설정합니다.
    /// @param _eyePosition 카메라 위치
    /// @param _viewProjMat 뷰-프로젝션 행렬
    void SetCamera(const Vec3& _eyePosition, const Mat4& _viewProjMat);

    /// @brief 변환 행렬을 설정합니다.
    /// @param _mat 변환 행렬
    void SetTransform(const Mat4& _mat);

    /// @brief 평행 이동을 설정합니다.
    /// @param _translate 평행 이동 벡터
    void SetTranslate(const Vec3& _translate);

    /// @brief 쿼터니언 회전을 설정합니다.
    /// @param _rotate 회전 쿼터니언
    void SetRotate(const Quat& _rotate);

    /// @brief 축과 각도로 회전을 설정합니다.
    /// @param _axis 회전축
    /// @param _angleRad 라디안 각도
    void SetRotate(const Vec3& _axis, float _angleRad);

    /// @brief 오일러 각도로 회전을 설정합니다.
    /// @param _yawPitchRollRad yaw(0), pitch(1), roll(2) 순서의 라디안 각도
    void SetRotate(const Vec3& _yawPitchRollRad);

    /// @brief 비균일 스케일링을 설정합니다.
    /// @param _scale 스케일 벡터
    void SetScale(const Vec3& _scale);

    /// @brief 현재 변환 행렬에 추가 변환을 곱합니다.
    /// @param _mat 곱할 변환 행렬
    void MultiplyTransform(const Mat4& _mat);

    /// @brief 현재 변환 행렬에 평행 이동을 곱합니다.
    /// @param _translate 평행 이동 벡터
    void MultiplyTranslate(const Vec3& _translate);

    /// @brief 현재 변환 행렬에 쿼터니언 회전을 곱합니다.
    /// @param _rotate 회전 쿼터니언
    void MultiplyRotate(const Quat& _rotate);

    /// @brief 현재 변환 행렬에 축-각도 회전을 곱합니다.
    /// @param _axis 회전축
    /// @param _angleRad 라디안 각도
    void MultiplyRotate(const Vec3& _axis, float _angleRad);

    /// @brief 현재 변환 행렬에 오일러 각도 회전을 곱합니다.
    /// @param _yawPitchRollRad yaw(0), pitch(1), roll(2) 순서의 라디안 각도
    void MultiplyRotate(const Vec3& _yawPitchRollRad);

    /// @brief 현재 변환 행렬에 스케일을 곱합니다.
    /// @param _scale 스케일 벡터
    void MultiplyScale(const Vec3& _scale);

    //////////////////////////////////////
    ///          RENDER STATE          ///
    //////////////////////////////////////

    /// @brief 현재 렌더링 상태를 스택에 저장합니다.
    void PushState();

    /// @brief 스택에서 렌더링 상태를 복원합니다.
    void PopState();

    /// @brief 도형의 색상을 설정합니다.
    /// @param _rgba 색상 값
    void SetColor(RGBA _rgba);

    /// @brief 깊이 테스트 및 쓰기 설정을 구성합니다.
    /// @param _bDepthTest 깊이 테스트 활성화 여부
    /// @param _bDepthWrite 깊이 쓰기 활성화 여부
    void SetDepthState(bool _bDepthTest, bool _bDepthWrite);

    /// @brief 채우기 모드를 설정합니다.
    /// @param _bWireframe true: 와이어프레임, false: 솔리드
    void SetWireframe(bool _bWireframe);

    /// @brief 도형의 세부 묘사 정도를 설정합니다.
    /// @param _level 분할 개수 품질 레벨
    void SetDetailLevel(eDetailLevel _level);

    ////////////////////////////////////
    ///          DRAW CALLS          ///
    ////////////////////////////////////

    /// 기본적으로 도형의 좌표는 월드 공간을 기준으로 선택합니다.
    /// 변환을 Push/Set 할 경우에는 달라질 수 있습니다.

    /// @brief 커서를 특정 위치로 이동합니다.
    /// @param _pos 이동할 위치
    /// @note 저수준 드로우 함수이므로 직접 사용하는걸 권장하지 않습니다.
    /// 다른 Draw*** 함수들을 사용하는걸 권장합니다.
    void MoveTo(const Vec3& _pos);

    /// @brief 현재 커서 위치에서 지정한 위치까지 선을 그립니다.
    /// @param _pos 선의 끝점
    /// @note 저수준 드로우 함수이므로 직접 사용하는걸 권장하지 않습니다.
    /// 다른 Draw*** 함수들을 사용하는걸 권장합니다.
    /// LineTo 를 연속으로 호출하면 선이 이어서 그려집니다.
    /// 원치 않다면 MoveTo 로 커서를 이동시킨 후 LineTo 를 호출하세요.
    void LineTo(const Vec3& _pos);

    /// @brief 선을 그립니다.
    /// @param _from 선의 시작점
    /// @param _to 선의 끝점
    void DrawLine(const Vec3& _from, const Vec3& _to);

    /// @brief 호를 그립니다.
    /// @param _center 호를 그릴 중심
    /// @param _radius 호의 반지름
    /// @param _normal 호가 놓일 평면의 법선 벡터
    /// @param _startAngleRad 시작 각
    /// @param _endAngleRad 끝 각
    /// @note 각도는 라디안 단위이며, 반시계 방향이 양의 방향입니다.
    void DrawArc(const Vec3& _center, float _radius, const Vec3& _normal, float _startAngleRad, float _endAngleRad);

    /// @brief 축 정렬 경계 상자(AABB)를 그립니다.
    /// @param _min 최소 좌표
    /// @param _max 최대 좌표
    void DrawAABB(const Vec3& _min, const Vec3& _max);

    /// @brief 회전된 경계 상자(OBB)를 그립니다.
    /// @param _center 중심점
    /// @param _size 크기
    /// @param _normal 상자의 윗면의 법선 벡터
    void DrawOBB(const Vec3& _center, const Vec3& _size, const Vec3& _normal);

    /// @brief 구를 그립니다.
    /// @param _center 중심점
    /// @param _radius 반지름
    void DrawSphere(const Vec3& _center, float _radius);

    /// @brief 법선 벡터를 기준으로 하는 사각형을 그립니다.
    /// @param _center 중심점
    /// @param _size 크기
    /// @param _normal 법선 벡터
    void DrawQuad(const Vec3& _center, const Vec2& _size, const Vec3& _normal);

    /// @brief 그리드(격자)를 그립니다.
    /// @param _center 중심점
    /// @param _size 크기
    /// @param _normal 법선 벡터
    /// @param _step 격자 간격
    void DrawGrid(const Vec3& _center, const Vec2& _size, const Vec3& _normal, float _step);

    /// @brief 법선 벡터를 기준으로 하는 원을 그립니다.
    /// @param _center 중심점
    /// @param _radius 반지름
    /// @param _normal 법선 벡터
    void DrawCircle(const Vec3& _center, float _radius, const Vec3& _normal);

    /// @brief 원뿔을 그립니다.
    /// @param _center 밑면 중심점
    /// @param _height 높이
    /// @param _radius 반지름
    /// @param _dir 밑면 -> 꼭대기 방향 벡터
    void DrawCone(const Vec3& _center, float _height, float _radius, const Vec3& _dir);

    /// @brief 절두체를 그립니다.
    /// @param _position 절두체 꼭대기의 위치 (= 카메라 위치)
    /// @param _forward 절두체의 앞 방향 벡터 (= 카메라 앞 방향)
    /// @param _projMat 뷰-프로젝션 행렬
    void DrawFrustum(const Vec3& _position, const Vec3& _forward, const Mat4& _projMat);

    /// @brief 원기둥을 그립니다.
    /// @param _center 밑면 중심점
    /// @param _height 높이
    /// @param _radius 반지름
    /// @param _dir 밑면 -> 윗면 방향 벡터
    void DrawCylinder(const Vec3& _center, float _height, float _radius, const Vec3& _dir);

    /// @brief 캡슐을 그립니다.
    /// @param _center 캡슐 한쪽 끝점
    /// @param _dir 캡슐 한쪽 끝점에서 다른 쪽 끝점으로 향하는 방향 벡터
    /// @param _height 높이
    /// @param _radius 반지름
    void DrawCapsule(const Vec3& _center, const Vec3& _dir, float _height, float _radius);

    /// @brief 메시를 그립니다.
    /// @param _mesh 그릴 메시
    /// @param _position 위치
    /// @param _forward 앞 방향 벡터
    /// @param _scale 스케일 벡터
    void DrawMesh(const Mesh& _mesh, const Vec3& _position, const Vec3& _forward, const Vec3& _scale);

}   // namespace debugdraw
};   // namespace jam
