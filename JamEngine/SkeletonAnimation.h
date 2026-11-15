#pragma once

namespace Jam
{

struct SkeletonNode
{
    String name;
    Int32  parentIndex     = -1;
    Mtx4   bindLocal       = Mtx4::kIdentity;
    Vec3   bindScale       = Vec3::kOne;
    Quat   bindRotation    = Quat::kIdentity;
    Vec3   bindTranslation = Vec3::kZero;
};

struct SkinJoint
{
    Int32 nodeIndex   = -1;
    Mtx4  inverseBind = Mtx4::kIdentity;
};

struct MeshSkin
{
    Int32             meshNodeIndex = -1;
    Vector<SkinJoint> joints;
};

struct Skeleton
{
    Vector<SkeletonNode> nodes;       // 부모가 자식보다 앞. 조인트 아닌 노드 포함
    Vector<MeshSkin>     meshSkins;   // 메쉬 인덱스와 동일
};

template<typename T>
struct AnimationKey
{
    double time  = 0.0;   // 초
    T      value = {};
};

struct AnimationChannel
{
    Int32                      nodeIndex = -1;
    Vector<AnimationKey<Vec3>> translations;
    Vector<AnimationKey<Quat>> rotations;
    Vector<AnimationKey<Vec3>> scales;
};

struct AnimationClip
{
    String                   name;
    double                   duration = 0.0;   // 초. 키는 시간순
    Vector<AnimationChannel> channels;
};

struct SkeletalAnimationData
{
    Skeleton              skeleton;
    Vector<AnimationClip> clips;
};

struct JointPose
{
    Vec3 scale       = Vec3::kOne;
    Quat rotation    = Quat::kIdentity;
    Vec3 translation = Vec3::kZero;
};

[[nodiscard]] Error SampleLocalPose(const Skeleton& _skeleton, const AnimationClip* _pClip, double _seconds, bool _bLoop, Vector<JointPose>& _poses);
void                BlendLocalPoses(Span<const JointPose> _a, Span<const JointPose> _b, float _t, Vector<JointPose>& _out);
[[nodiscard]] Error LocalPosesToGlobals(const Skeleton& _skeleton, Span<const JointPose> _poses, Vector<Mtx4>& _globals);

// 다른 스켈레톤의 클립을 노드 이름으로 _dst에 재연결. 대응 노드가 없는 채널은 제외
// _rootName이 있으면 해당 노드만 translation 유지 (바인드 높이 비율로 스케일)
// 나머지 조인트는 회전/스케일만 사용 (_dst의 뼈 길이 유지)
[[nodiscard]] AnimationClip RetargetClipByName(const Skeleton& _src, const AnimationClip& _clip, const Skeleton& _dst, StringView _rootName = {});

// 노드의 수평 translation을 첫 키로 고정
void LockHorizontalTranslation(AnimationClip& _clip, Int32 _nodeIndex);

// _pClip이 nullptr이면 바인드 포즈. 출력 버퍼는 프레임 간 재사용
[[nodiscard]] Error SampleSkeletonPose(const Skeleton& _skeleton, const AnimationClip* _pClip, double _seconds, bool _bLoop, Vector<Mtx4>& _globals);
[[nodiscard]] Error BuildSkinningPalette(const MeshSkin& _skin, Span<const Mtx4> _globals, Vector<Mtx4>& _palette);

inline constexpr Int32 kMaxSkinJoints = 256;

}   // namespace Jam
