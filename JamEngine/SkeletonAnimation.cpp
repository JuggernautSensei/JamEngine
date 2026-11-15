#include "pch.h"
#include "SkeletonAnimation.h"
#include <cmath>

namespace Jam
{
namespace
{
    template<typename T>
    T SampleKeys_(const Vector<AnimationKey<T>>& _keys, const double _time, const T& _fallback)
    {
        if (_keys.empty())
            return _fallback;
        if (_time <= _keys.front().time || _keys.size() == 1)
            return _keys.front().value;
        if (_time >= _keys.back().time)
            return _keys.back().value;
        const auto  right = std::upper_bound(_keys.begin(), _keys.end(), _time, [](double _t, const auto& _key) { return _t < _key.time; });
        const auto& a     = *(right - 1);
        const auto& b     = *right;
        const float t     = static_cast<float>((_time - a.time) / (b.time - a.time));
        if constexpr (std::is_same_v<T, Quat>)
            return a.value.Slerp(b.value, t).Normalized();
        else
            return a.value + (b.value - a.value) * t;
    }
}   // namespace

Error SampleLocalPose(const Skeleton& _skeleton, const AnimationClip* _pClip, double _seconds, const bool _bLoop, Vector<JointPose>& _poses)
{
    if (_skeleton.nodes.empty() || !std::isfinite(_seconds))
        return Error::InvalidArgument;
    if (_pClip)
    {
        if (!std::isfinite(_pClip->duration) || _pClip->duration < 0.0)
            return Error::InvalidData;
        if (_bLoop && _pClip->duration > 0.0)
        {
            _seconds = std::fmod(_seconds, _pClip->duration);
            if (_seconds < 0.0)
                _seconds += _pClip->duration;
        }
        else
            _seconds = std::clamp(_seconds, 0.0, _pClip->duration);
    }

    _poses.resize(_skeleton.nodes.size());
    for (Int32 i = 0; i < _skeleton.nodes.size(); ++i)
    {
        const SkeletonNode& node = _skeleton.nodes[i];
        _poses[i]                = { node.bindScale, node.bindRotation, node.bindTranslation };
    }
    if (_pClip)
    {
        for (const AnimationChannel& channel: _pClip->channels)
        {
            if (channel.nodeIndex < 0 || channel.nodeIndex >= _skeleton.nodes.size())
                return Error::InvalidData;
            const SkeletonNode& node = _skeleton.nodes[channel.nodeIndex];
            JointPose&          pose = _poses[channel.nodeIndex];
            pose.scale               = SampleKeys_(channel.scales, _seconds, node.bindScale);
            pose.rotation            = SampleKeys_(channel.rotations, _seconds, node.bindRotation);
            pose.translation         = SampleKeys_(channel.translations, _seconds, node.bindTranslation);
        }
    }
    return Error::None;
}

void BlendLocalPoses(const Span<const JointPose> _a, const Span<const JointPose> _b, const float _t, Vector<JointPose>& _out)
{
    _out.resize(_a.size());
    for (SizeT i = 0; i < _a.size(); ++i)
    {
        _out[i].scale       = _a[i].scale.Lerp(_b[i].scale, _t);
        _out[i].translation = _a[i].translation.Lerp(_b[i].translation, _t);
        _out[i].rotation    = _a[i].rotation.Slerp(_b[i].rotation, _t).Normalized();
    }
}

Error LocalPosesToGlobals(const Skeleton& _skeleton, const Span<const JointPose> _poses, Vector<Mtx4>& _globals)
{
    if (_poses.size() != _skeleton.nodes.size())
        return Error::InvalidArgument;
    _globals.resize(_poses.size());
    for (Int32 i = 0; i < _skeleton.nodes.size(); ++i)
    {
        const Int32 parent = _skeleton.nodes[i].parentIndex;
        if (parent < -1 || parent >= i)
            return Error::InvalidData;
        _globals[i] = Mtx4::MakeSRT(_poses[i].scale, _poses[i].rotation, _poses[i].translation);
        if (parent >= 0)
            _globals[i] *= _globals[parent];
    }
    return Error::None;
}

AnimationClip RetargetClipByName(const Skeleton& _src, const AnimationClip& _clip, const Skeleton& _dst, const StringView _rootName)
{
    HashMap<String, Int32> dstIndexByName;
    for (Int32 i = 0; i < _dst.nodes.size(); ++i)
        dstIndexByName.emplace(_dst.nodes[i].name, i);

    AnimationClip result = {};
    result.name          = _clip.name;
    result.duration      = _clip.duration;
    for (const AnimationChannel& channel: _clip.channels)
    {
        if (channel.nodeIndex < 0 || channel.nodeIndex >= _src.nodes.size())
            continue;
        const auto it = dstIndexByName.find(_src.nodes[channel.nodeIndex].name);
        if (it == dstIndexByName.end())
            continue;
        AnimationChannel& out = result.channels.emplace_back(channel);
        out.nodeIndex         = it->second;
        if (_rootName.IsEmpty())
            continue;

        const SkeletonNode& srcNode = _src.nodes[channel.nodeIndex];
        if (StringView { srcNode.name.GetPtr(), srcNode.name.GetLength() } != _rootName)
        {
            out.translations.clear();
            continue;
        }
        const float srcHeight = srcNode.bindTranslation.y;
        const float ratio     = std::abs(srcHeight) > 1e-4f ? _dst.nodes[it->second].bindTranslation.y / srcHeight : 1.f;
        for (AnimationKey<Vec3>& key: out.translations)
            key.value *= ratio;
    }
    return result;
}

void LockHorizontalTranslation(AnimationClip& _clip, const Int32 _nodeIndex)
{
    for (AnimationChannel& channel: _clip.channels)
    {
        if (channel.nodeIndex != _nodeIndex || channel.translations.empty())
            continue;
        const Vec3 first = channel.translations.front().value;
        for (AnimationKey<Vec3>& key: channel.translations)
        {
            key.value.x = first.x;
            key.value.z = first.z;
        }
    }
}

Error SampleSkeletonPose(const Skeleton& _skeleton, const AnimationClip* _pClip, double _seconds, const bool _bLoop, Vector<Mtx4>& _globals)
{
    Vector<JointPose> poses;
    if (const Error err = SampleLocalPose(_skeleton, _pClip, _seconds, _bLoop, poses))
        return err;
    return LocalPosesToGlobals(_skeleton, poses, _globals);
}

Error BuildSkinningPalette(const MeshSkin& _skin, const Span<const Mtx4> _globals, Vector<Mtx4>& _palette)
{
    if (_skin.meshNodeIndex < 0 || _skin.meshNodeIndex >= _globals.size() || _skin.joints.size() > kMaxSkinJoints)
        return Error::InvalidArgument;
    const Mtx4 invMesh = _globals[_skin.meshNodeIndex].Inverse();
    for (float value: invMesh.m)
        if (!std::isfinite(value))
            return Error::InvalidData;
    _palette.resize(_skin.joints.size());
    for (Int32 i = 0; i < _skin.joints.size(); ++i)
    {
        const SkinJoint& joint = _skin.joints[i];
        if (joint.nodeIndex < 0 || joint.nodeIndex >= _globals.size())
            return Error::InvalidData;
        _palette[i] = joint.inverseBind * _globals[joint.nodeIndex] * invMesh;
    }
    return Error::None;
}
}   // namespace Jam
