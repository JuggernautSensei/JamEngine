#include "pch.h"
#include "../JamEngine/ModelImporter.h"

TEST(AnimationImport, NodeTranslationUsesEngineRowVectors)
{
    constexpr char kGltf[]  = R"json({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"name":"Translated","translation":[2,3,4]}]})json";
    auto           importer = AssimpImporter::ImportFromMemory({ kGltf, sizeof(kGltf) - 1 }, eAssimpImporterFlags::LoadMeshes);
    ASSERT_TRUE(importer);
    const ModelNode& root = importer->GetRoot();
    const ModelNode& node = root.children.empty() ? root : root.children[0];
    EXPECT_NEAR(node.localMtx.r3.x, 2.f, 1e-5f);
    EXPECT_NEAR(node.localMtx.r3.y, 3.f, 1e-5f);
    EXPECT_NEAR(node.localMtx.r3.z, -4.f, 1e-5f);
}

namespace
{

Vec3 TransformPoint(const Mtx4& _m, const Vec3& _p)
{
    const Vec4 r = _m.r0 * _p.x + _m.r1 * _p.y + _m.r2 * _p.z + _m.r3;
    return { r.x, r.y, r.z };
}

}   // namespace

TEST(AnimationImport, MixamoSkinnedBoundsStayCharacterSized)
{
    constexpr char kPath[]  = "SandboxApp/Resources/SkinningScene/Paladin_Walk.fbx";
    auto           importer = AssimpImporter::ImportFromFile(kPath, eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::LoadAnimations);
    if (!importer)
        GTEST_SKIP() << "sample FBX not found from the current directory";

    const SkeletalAnimationData& data = importer->GetAnimationData();
    ASSERT_FALSE(data.clips.empty());
    ASSERT_EQ(data.skeleton.meshSkins.size(), importer->GetMeshes().size());

    for (const double time: { 0.0, data.clips[0].duration * 0.5 })
    {
        Vector<Mtx4> globals;
        ASSERT_FALSE(SampleSkeletonPose(data.skeleton, &data.clips[0], time, true, globals));

        float minY = 1e30f, maxY = -1e30f, minX = 1e30f, maxX = -1e30f;
        for (SizeT m = 0; m < importer->GetMeshes().size(); ++m)
        {
            const MeshSkin& skin = data.skeleton.meshSkins[m];
            Vector<Mtx4>    palette;
            ASSERT_FALSE(BuildSkinningPalette(skin, globals, palette));
            const ImportedMesh& mesh = importer->GetMeshes()[m];
            for (SizeT v = 0; v < mesh.positions.size(); ++v)
            {
                Mtx4 skinMtx = Mtx4::kIdentity;
                skinMtx.r0 = skinMtx.r1 = skinMtx.r2 = skinMtx.r3 = Vec4 { 0.f, 0.f, 0.f, 0.f };
                for (Int32 k = 0; k < 4; ++k)
                {
                    const float w = mesh.blendWeights[v][k];
                    const Mtx4& j = palette[mesh.blendIndices[v][k]];
                    skinMtx.r0 += j.r0 * w;
                    skinMtx.r1 += j.r1 * w;
                    skinMtx.r2 += j.r2 * w;
                    skinMtx.r3 += j.r3 * w;
                }
                const Vec3 p = TransformPoint(globals[skin.meshNodeIndex], TransformPoint(skinMtx, mesh.positions[v]));
                minY         = std::min(minY, p.y);
                maxY         = std::max(maxY, p.y);
                minX         = std::min(minX, p.x);
                maxX         = std::max(maxX, p.x);
            }
        }
        printf("t=%.3f  x[%.2f, %.2f]  y[%.2f, %.2f]\n", time, minX, maxX, minY, maxY);
    }
}

TEST(AnimationBlend, HalfwayBlendsTranslationAndRotation)
{
    const Vector<JointPose> a = {
        { Vec3::kOne, Quat::kIdentity, Vec3 { 0.f, 0.f, 0.f } }
    };
    const Vector<JointPose> b = {
        { Vec3::kOne, Quat::FromAxisAngle(Vec3 { 0.f, 1.f, 0.f }, ToRad(90.f)), Vec3 { 2.f, 0.f, 0.f } }
    };
    Vector<JointPose> out;
    BlendLocalPoses(a, b, 0.5f, out);
    EXPECT_NEAR(out[0].translation.x, 1.f, 1e-5f);
    EXPECT_TRUE(out[0].rotation.IsEqualApprox(Quat::FromAxisAngle(Vec3 { 0.f, 1.f, 0.f }, ToRad(45.f))));
}

TEST(AnimationRetarget, RemapsChannelsByNodeNameAndDropsUnknown)
{
    Skeleton src;
    src.nodes.resize(3);
    src.nodes[0].name = "Root";
    src.nodes[1].name = "Hips";
    src.nodes[2].name = "Extra";
    Skeleton dst;
    dst.nodes.resize(2);
    dst.nodes[0].name = "Hips";
    dst.nodes[1].name = "Root";

    AnimationClip clip;
    clip.duration = 1.0;
    clip.channels.resize(2);
    clip.channels[0].nodeIndex = 1;
    clip.channels[1].nodeIndex = 2;

    const AnimationClip result = RetargetClipByName(src, clip, dst);
    ASSERT_EQ(result.channels.size(), 1u);
    EXPECT_EQ(result.channels[0].nodeIndex, 0);
}
