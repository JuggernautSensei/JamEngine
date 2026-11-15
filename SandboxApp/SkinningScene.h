#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"
#include "../JamEngine/SkeletonAnimation.h"

class SkinningScene : public Scene
{
    JAM_SCENE_BODY(SkinningScene);

    enum class eCharacterState
    {
        Idle,
        Walk,
        Slash,
        Count
    };

    struct Npc
    {
        eCharacterState state    = eCharacterState::Idle;
        Vec3            center   = Vec3::kZero;
        float           radius   = 0.f;
        float           phase    = 0.f;
        float           yaw      = 0.f;
        Vec3            position = Vec3::kZero;
        Vector<Mtx4>    globals  = {};
    };

public:
    SkinningScene();
    ~SkinningScene() noexcept override;

    SkinningScene(const SkinningScene&)                = delete;
    SkinningScene& operator=(const SkinningScene&)     = delete;
    SkinningScene(SkinningScene&&) noexcept            = delete;
    SkinningScene& operator=(SkinningScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitModel_();
    void InitRenderGraph_();
    void LoadStateClips_();
    void UpdateInput_(float _deltaTime);
    void ChangeState_(eCharacterState _state);
    void UpdatePose_(float _deltaTime);
    void InitNpcs_();
    void UpdateNpcs_();
    void DrawCharacter_(const Vector<Mtx4>& _globals, const Mtx4& _world);
    void Submit_();

    [[nodiscard]] const AnimationClip* GetStateClip_(eCharacterState _state) const;
    [[nodiscard]] Mtx4                 CalcCharacterWorld_() const;
    [[nodiscard]] static Mtx4          MakeCharacterWorld_(Vec3 _position, float _yaw);

    RenderGraph m_graph = {};

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;
    ConstantBufferHandle m_skinCbh   = kInvalidHandle;

    Ref<Model>            m_pModel = nullptr;
    SkeletalAnimationData m_animation;
    Vector<Int32>         m_meshMaterials;
    Vector<JointPose>     m_currentPoses;
    Vector<JointPose>     m_previousPoses;
    Vector<JointPose>     m_blendedPoses;
    Vector<Mtx4>          m_globals;
    Vector<Mtx4>          m_palette;
    Vector<Npc>           m_npcs;
    Vector<JointPose>     m_npcPoses;
    Ref<Mesh>             m_pFloor        = nullptr;
    Ref<Texture>          m_pFloorTexture = nullptr;
    double                m_npcTime       = 0.0;

    Array<AnimationClip, 3> m_stateClips;
    Array<bool, 3>          m_bHasStateClip = {};

    FlyingCamera m_camera = {};

    eCharacterState m_state         = eCharacterState::Idle;
    eCharacterState m_previousState = eCharacterState::Idle;
    double          m_stateTime     = 0.0;
    double          m_previousTime  = 0.0;
    float           m_blendElapsed  = 1.f;

    Vec3  m_position = Vec3::kZero;
    float m_yaw      = 0.f;
};
