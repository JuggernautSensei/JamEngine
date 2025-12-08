#pragma once
//
//#include "BoundingBox.h"
//#include "VisibleItem.h"
//#include "Timer.h"
//#include "vendor/bvh/v2/bbox.h"
//#include "vendor/bvh/v2/default_builder.h"
//#include "vendor/bvh/v2/node.h"
//#include "vendor/bvh/v2/stack.h"
//#include "vendor/bvh/v2/thread_pool.h"
//#include "vendor/bvh/v2/vec.h"
//
//namespace jam
//{
//
//class Transform;
//class TransformHierarchy;
//class Scene;
//struct Renderable;
//
//class Bvh
//{
//    using Scalar         = float;
//    using BVec           = bvh::v2::Vec<Scalar, 3>;
//    using BBox           = bvh::v2::BBox<Scalar, 3>;
//    using BNode          = bvh::v2::Node<Scalar, 3>;
//    using BVH            = bvh::v2::Bvh<BNode>;
//    using BThreadPool    = bvh::v2::ThreadPool;
//    using BExecutor      = bvh::v2::ParallelExecutor;
//    using BBuilder       = bvh::v2::DefaultBuilder<BNode>;
//    using BBuilderConfig = BBuilder::Config;
//    using BBVHIndex      = BVH::Index;
//
//    struct Item
//    {
//        const Renderable* pRenderable = nullptr;
//        const Transform*  pTransform  = nullptr;
//        AABB              aabb        = {};
//    };
//
//public:
//    Bvh();
//
//    ~Bvh()                         = default;
//    Bvh(const Bvh&)                = delete;
//    Bvh& operator=(const Bvh&)     = delete;
//    Bvh(Bvh&&) noexcept            = delete;
//    Bvh& operator=(Bvh&&) noexcept = delete;
//
//    void Build(const Scene& _scene);
//    void Refit();
//
//    void QueryFrustum(const Frustum& _frustum, std::vector<VisibleItem>& _outItems);
//
//private:
//    [[nodiscard]] bool Intersect_(const BNode& _node, const Frustum& _frustum) const;
//
//    BVH         m_bvh;
//    BThreadPool m_threadPool;
//    BExecutor   m_executor;
//
//    std::vector<BBox> m_boxes;
//    std::vector<BVec> m_centers;
//
//    std::vector<Item>   m_items;
//    std::vector<UInt32> m_itemIDs;
//
//    std::vector<UInt32>    m_tmpIDs;
//    std::vector<BBVHIndex> m_tmpStack;
//
//    // profile
//    float m_lastBuildSec        = 0.f;
//    float m_lastRebuiltSec      = 0.f;
//    float m_lastFrustumQuerySec = 0.f;
//    Timer m_timer               = {};
//};
//
//}   // namespace jam
