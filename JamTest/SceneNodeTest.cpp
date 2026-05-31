
#include "pch.h"

namespace
{

class TestScene final : public Scene
{
public:
    StringView GetTypeName() const override { return "TestScene"; }
    UInt32     GetTypeHash() const override { return 0x12345678u; }
};

void UpdateEntity(Entity e, Vector<Entity>& updated)
{
    e.GetSceneNode().UpdateTree(updated);
}

}   // namespace

TEST(SceneNodeTest, RootNode_Basics)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    const Entity      root  = scene->GetRootEntity();

    const SceneNode& n = root.GetSceneNode();
    EXPECT_TRUE(n.IsRoot());
    EXPECT_FALSE(n.HasParent());
    EXPECT_FALSE(n.HasChildren());
    EXPECT_EQ(n.GetRoot(), root);
    EXPECT_EQ(n.GetOwner(), root);
    EXPECT_FALSE(n.GetParent());
    EXPECT_FALSE(n.GetFirstChild());
    EXPECT_FALSE(n.GetNextSibling());
    EXPECT_FALSE(n.GetPrevSibling());
}

TEST(SceneNodeTest, SetParent_AndSiblingLinks)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();

    Entity a = scene->CreateEntity("a");
    Entity b = scene->CreateEntity("b");
    Entity c = scene->CreateEntity("c");

    a.GetSceneNode().SetParent(root);
    b.GetSceneNode().SetParent(root);
    c.GetSceneNode().SetParent(root);

    const SceneNode& rootN = root.GetSceneNode();
    EXPECT_TRUE(rootN.HasChildren());
    EXPECT_EQ(rootN.GetFirstChild(), c);

    const SceneNode& cN = c.GetSceneNode();
    EXPECT_EQ(cN.GetNextSibling(), b);
    EXPECT_FALSE(cN.GetPrevSibling());

    const SceneNode& bN = b.GetSceneNode();
    EXPECT_EQ(bN.GetNextSibling(), a);
    EXPECT_EQ(bN.GetPrevSibling(), c);

    const SceneNode& aN = a.GetSceneNode();
    EXPECT_FALSE(aN.GetNextSibling());
    EXPECT_EQ(aN.GetPrevSibling(), b);
}

TEST(SceneNodeTest, Orphanize_ReleasesFromSiblingChain)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();

    Entity a = scene->CreateEntity("a");
    Entity b = scene->CreateEntity("b");
    Entity c = scene->CreateEntity("c");
    a.GetSceneNode().SetParent(root);
    b.GetSceneNode().SetParent(root);
    c.GetSceneNode().SetParent(root);

    b.GetSceneNode().Orphanize();

    const SceneNode& rootN = root.GetSceneNode();
    EXPECT_EQ(rootN.GetFirstChild(), c);
    EXPECT_EQ(c.GetSceneNode().GetNextSibling(), a);
    EXPECT_FALSE(a.GetSceneNode().GetNextSibling());
    EXPECT_EQ(a.GetSceneNode().GetPrevSibling(), c);

    const SceneNode& bN = b.GetSceneNode();
    EXPECT_FALSE(bN.HasParent());
    EXPECT_FALSE(bN.GetNextSibling());
    EXPECT_FALSE(bN.GetPrevSibling());
}

TEST(SceneNodeTest, AncestorDescendantQueries)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();

    Entity parent = scene->CreateEntity("parent");
    Entity child  = scene->CreateEntity("child");
    Entity grand  = scene->CreateEntity("grand");

    parent.GetSceneNode().SetParent(root);
    child.GetSceneNode().SetParent(parent);
    grand.GetSceneNode().SetParent(child);

    EXPECT_TRUE(grand.GetSceneNode().IsDescendantOf(child));
    EXPECT_TRUE(grand.GetSceneNode().IsDescendantOf(parent));
    EXPECT_TRUE(grand.GetSceneNode().IsDescendantOf(root));
    EXPECT_FALSE(parent.GetSceneNode().IsDescendantOf(child));

    EXPECT_TRUE(root.GetSceneNode().IsAncestorOf(grand));
    EXPECT_TRUE(parent.GetSceneNode().IsAncestorOf(grand));
    EXPECT_FALSE(child.GetSceneNode().IsAncestorOf(parent));
}

TEST(SceneNodeTest, UpdateSceneGraph_LocalToWorldPropagation)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();
    Entity            p     = scene->CreateEntity("p");
    Entity            c     = scene->CreateEntity("c");
    p.GetSceneNode().SetParent(root);
    c.GetSceneNode().SetParent(p);

    p.ModifyLocalTransform().SetTranslation(10.0f, 0.0f, 0.0f);
    c.ModifyLocalTransform().SetTranslation(0.0f, 5.0f, 0.0f);

    Vector<Entity> updated;
    UpdateEntity(root, updated);
    UpdateEntity(p, updated);
    UpdateEntity(c, updated);

    const Vec3 worldP = p.GetWorldTransform().GetTranslation();
    const Vec3 worldC = c.GetWorldTransform().GetTranslation();
    EXPECT_TRUE(IsEqualApprox(worldP, { 10.0f, 0.0f, 0.0f }));
    EXPECT_TRUE(IsEqualApprox(worldC, { 10.0f, 5.0f, 0.0f }));
}

TEST(SceneNodeTest, UpdateSceneGraph_WorldToLocalPropagation)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();
    Entity            p     = scene->CreateEntity("p");
    Entity            c     = scene->CreateEntity("c");
    p.GetSceneNode().SetParent(root);
    c.GetSceneNode().SetParent(p);

    p.ModifyWorldTransform().SetTranslation(10.0f, 0.0f, 0.0f);
    c.ModifyWorldTransform().SetTranslation(10.0f, 5.0f, 0.0f);

    Vector<Entity> updated;
    UpdateEntity(root, updated);
    UpdateEntity(p, updated);
    UpdateEntity(c, updated);

    EXPECT_TRUE(IsEqualApprox(p.GetLocalTransform().GetTranslation(), { 10.0f, 0.0f, 0.0f }));
    EXPECT_TRUE(IsEqualApprox(c.GetLocalTransform().GetTranslation(), { 0.0f, 5.0f, 0.0f }));
}

TEST(SceneNodeTest, SetParent_KeepLocalVsKeepWorld)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();

    Entity a = scene->CreateEntity("a");
    Entity b = scene->CreateEntity("b");
    a.GetSceneNode().SetParent(root);
    b.GetSceneNode().SetParent(root);

    a.ModifyLocalTransform().SetTranslation(10.0f, 0.0f, 0.0f);
    b.ModifyLocalTransform().SetTranslation(100.0f, 0.0f, 0.0f);

    Vector<Entity> updated;
    UpdateEntity(a, updated);
    UpdateEntity(b, updated);

    const Vec3 worldBefore = a.GetWorldTransform().GetTranslation();
    EXPECT_TRUE(IsEqualApprox(worldBefore, { 10.0f, 0.0f, 0.0f }));

    a.GetSceneNode().SetParent(b, true);   // keep local
    UpdateEntity(a, updated);
    UpdateEntity(b, updated);
    const Vec3 worldKeepLocal = a.GetWorldTransform().GetTranslation();
    EXPECT_TRUE(IsEqualApprox(worldKeepLocal, { 110.0f, 0.0f, 0.0f }));

    a.GetSceneNode().SetParent(root);
    UpdateEntity(a, updated);
    UpdateEntity(b, updated);
    a.ModifyWorldTransform().SetTranslation(10.0f, 0.0f, 0.0f);
    UpdateEntity(a, updated);

    a.GetSceneNode().SetParent(b, false);   // keep world
    UpdateEntity(a, updated);
    UpdateEntity(b, updated);
    EXPECT_TRUE(IsEqualApprox(a.GetWorldTransform().GetTranslation(), { 10.0f, 0.0f, 0.0f }));
    EXPECT_TRUE(IsEqualApprox(a.GetLocalTransform().GetTranslation(), { -90.0f, 0.0f, 0.0f }));
}

TEST(SceneNodeTest, UpdateSceneGraph_ChildDirtyRespectsChildWorldEdit)
{
    Scoped<TestScene> scene = MakeScoped<TestScene>();
    Entity            root  = scene->GetRootEntity();
    Entity            p     = scene->CreateEntity("p");
    Entity            c     = scene->CreateEntity("c");
    p.GetSceneNode().SetParent(root);
    c.GetSceneNode().SetParent(p);

    p.ModifyLocalTransform().SetTranslation(10.0f, 0.0f, 0.0f);
    c.ModifyLocalTransform().SetTranslation(0.0f, 5.0f, 0.0f);

    Vector<Entity> updated;
    UpdateEntity(p, updated);
    UpdateEntity(c, updated);

    c.ModifyWorldTransform().SetTranslation(999.0f, 5.0f, 0.0f);
    p.ModifyLocalTransform().SetTranslation(20.0f, 0.0f, 0.0f);

    UpdateEntity(p, updated);
    UpdateEntity(c, updated);

    EXPECT_TRUE(IsEqualApprox(c.GetWorldTransform().GetTranslation(), { 999.0f, 5.0f, 0.0f }));
    EXPECT_TRUE(IsEqualApprox(c.GetLocalTransform().GetTranslation(), { 979.0f, 5.0f, 0.0f }));
}
