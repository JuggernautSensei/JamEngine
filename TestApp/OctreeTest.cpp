#include "pch.h"

//#include "OctreeTest.h"
//#include "Octree.h"
//#include <random>
//#include <iomanip>
//
//static std::random_device              g_rd;
//static std::mt19937                    g_gen(g_rd());
//static std::uniform_real_distribution<> g_dis(0.0, 1.0);
//static UInt64                           g_entityCounter = 1;
//
//// Helper to create a mock registry for testing
//static entt::registry g_testRegistry;
//
//void OctreeTest::PrintTestResult(const char* _testName, bool _passed)
//{
//    if (_passed)
//    {
//        std::cout << "[PASS] " << _testName << std::endl;
//    }
//    else
//    {
//        std::cout << "[FAIL] " << _testName << std::endl;
//    }
//}
//
//Entity OctreeTest::CreateTestEntity()
//{
//    entt::entity handle = g_testRegistry.create();
//    UID uid = g_entityCounter++;
//    return Entity(g_testRegistry, handle, uid);
//}
//
//Aabb OctreeTest::CreateRandomAabb(Vec3 _center, Vec3 _minExtents, Vec3 _maxExtents)
//{
//    Vec3 extents;
//    extents.x = RandomFloat(_minExtents.x, _maxExtents.x);
//    extents.y = RandomFloat(_minExtents.y, _maxExtents.y);
//    extents.z = RandomFloat(_minExtents.z, _maxExtents.z);
//    
//    return Aabb::FromCenterExtents(_center, extents);
//}
//
//Vec3 OctreeTest::RandomVec3(float _min, float _max)
//{
//    return Vec3(
//        RandomFloat(_min, _max),
//        RandomFloat(_min, _max),
//        RandomFloat(_min, _max)
//    );
//}
//
//float OctreeTest::RandomFloat(float _min, float _max)
//{
//    return _min + static_cast<float>(g_dis(g_gen)) * (_max - _min);
//}
//
//// ============================================================================
//// Validity Tests
//// ============================================================================
//
//bool OctreeTest::TestBasicInsertAndQuery()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 5;
//    desc.subdivisionThreshold = 4;
//    
//    Octree<Int32> octree(desc);
//    
//    Entity entity = CreateTestEntity();
//    Aabb aabb = Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(5, 5, 5));
//    
//    octree.Insert(entity, aabb, 42);
//    
//    // Query with a bounding volume that should contain the inserted object
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(50, 50, 50)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) {
//            foundCount++;
//            return _e == entity && _data == 42;
//        }
//    );
//    
//    return foundCount == 1;
//}
//
//bool OctreeTest::TestInsertMultipleObjects()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    
//    Octree<Int32> octree(desc);
//    
//    const Int32 numObjects = 10;
//    std::vector<Entity> entities;
//    
//    for (Int32 i = 0; i < numObjects; ++i)
//    {
//        Entity entity = CreateTestEntity();
//        Vec3 center = RandomVec3(-50.f, 50.f);
//        Aabb aabb = CreateRandomAabb(center, Vec3(1, 1, 1), Vec3(5, 5, 5));
//        
//        octree.Insert(entity, aabb, i);
//        entities.push_back(entity);
//    }
//    
//    // Query all
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(200, 200, 200)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == numObjects;
//}
//
//bool OctreeTest::TestUpdateObject()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    
//    Octree<Int32> octree(desc);
//    
//    Entity entity = CreateTestEntity();
//    Aabb initialAabb = Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(5, 5, 5));
//    
//    octree.Insert(entity, initialAabb, 123);
//    
//    // Update position
//    Aabb newAabb = Aabb::FromCenterExtents(Vec3(50, 50, 50), Vec3(5, 5, 5));
//    octree.Update(entity, newAabb);
//    
//    // Query old position - should find nothing
//    Int32 foundAtOld = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(10, 10, 10)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundAtOld++; }
//    );
//    
//    // Query new position - should find the object
//    Int32 foundAtNew = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(50, 50, 50), Vec3(10, 10, 10)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundAtNew++; }
//    );
//    
//    return foundAtOld == 0 && foundAtNew == 1;
//}
//
//bool OctreeTest::TestRemoveObject()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    
//    Octree<Int32> octree(desc);
//    
//    Entity entity = CreateTestEntity();
//    Aabb aabb = Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(5, 5, 5));
//    
//    octree.Insert(entity, aabb, 99);
//    octree.Remove(entity);
//    
//    // Query - should find nothing
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(50, 50, 50)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == 0;
//}
//
//bool OctreeTest::TestOctreeExpansion()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(10.f);
//    desc.maxDepth = 5;
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert object within initial bounds
//    Entity entity1 = CreateTestEntity();
//    octree.Insert(entity1, Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(2, 2, 2)), 1);
//    
//    // Insert object outside initial bounds - should trigger expansion
//    Entity entity2 = CreateTestEntity();
//    octree.Insert(entity2, Aabb::FromCenterExtents(Vec3(100, 100, 100), Vec3(2, 2, 2)), 2);
//    
//    // Query both objects
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(50, 50, 50), Vec3(200, 200, 200)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == 2;
//}
//
//bool OctreeTest::TestOctreeSubdivision()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 8;
//    desc.subdivisionThreshold = 4;
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert enough objects in a small area to trigger subdivision
//    const Int32 numObjects = 10;
//    for (Int32 i = 0; i < numObjects; ++i)
//    {
//        Entity entity = CreateTestEntity();
//        Vec3 center = Vec3(
//            RandomFloat(-5.f, 5.f),
//            RandomFloat(-5.f, 5.f),
//            RandomFloat(-5.f, 5.f)
//        );
//        Aabb aabb = CreateRandomAabb(center, Vec3(0.5f, 0.5f, 0.5f), Vec3(1.f, 1.f, 1.f));
//        octree.Insert(entity, aabb, i);
//    }
//    
//    // Query the clustered area
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(10, 10, 10)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == numObjects;
//}
//
//bool OctreeTest::TestOctreeShrinkToFit()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(10.f);
//    desc.maxDepth = 5;
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert object far away to expand the tree
//    Entity entity1 = CreateTestEntity();
//    octree.Insert(entity1, Aabb::FromCenterExtents(Vec3(100, 100, 100), Vec3(2, 2, 2)), 1);
//    
//    // Remove it
//    octree.Remove(entity1);
//    
//    // Insert object close to origin
//    Entity entity2 = CreateTestEntity();
//    octree.Insert(entity2, Aabb::FromCenterExtents(Vec3(5, 5, 5), Vec3(1, 1, 1)), 2);
//    
//    // Shrink the tree
//    octree.ShrinkToFit();
//    
//    // Query should still find the object
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(50, 50, 50)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == 1;
//}
//
//bool OctreeTest::TestBoundaryConditions()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 5;
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert objects at various boundary positions
//    std::vector<Entity> entities;
//    Vec3 positions[] = {
//        Vec3(0, 0, 0),          // Center
//        Vec3(100, 0, 0),        // +X boundary
//        Vec3(-100, 0, 0),       // -X boundary
//        Vec3(0, 100, 0),        // +Y boundary
//        Vec3(0, -100, 0),       // -Y boundary
//        Vec3(0, 0, 100),        // +Z boundary
//        Vec3(0, 0, -100),       // -Z boundary
//        Vec3(100, 100, 100),    // Corner
//        Vec3(-100, -100, -100)  // Opposite corner
//    };
//    
//    for (Vec3 pos : positions)
//    {
//        Entity entity = CreateTestEntity();
//        octree.Insert(entity, Aabb::FromCenterExtents(pos, Vec3(2, 2, 2)), 1);
//        entities.push_back(entity);
//    }
//    
//    // Query all
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(500, 500, 500)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == 9;
//}
//
//bool OctreeTest::TestLargeObjectHandling()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 5;
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert a very large object that spans multiple octree nodes
//    Entity largeEntity = CreateTestEntity();
//    octree.Insert(largeEntity, Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(50, 50, 50)), 1);
//    
//    // Insert smaller objects
//    for (Int32 i = 0; i < 5; ++i)
//    {
//        Entity entity = CreateTestEntity();
//        Vec3 center = RandomVec3(-40.f, 40.f);
//        octree.Insert(entity, CreateRandomAabb(center, Vec3(1, 1, 1), Vec3(3, 3, 3)), i + 2);
//    }
//    
//    // Query should find all objects
//    Int32 foundCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(100, 100, 100)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    
//    return foundCount == 6;
//}
//
//bool OctreeTest::TestQueryWithDifferentVolumes()
//{
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    
//    Octree<Int32> octree(desc);
//    
//    // Insert objects at known positions
//    Entity e1 = CreateTestEntity();
//    Entity e2 = CreateTestEntity();
//    Entity e3 = CreateTestEntity();
//    
//    octree.Insert(e1, Aabb::FromCenterExtents(Vec3(-50, 0, 0), Vec3(5, 5, 5)), 1);
//    octree.Insert(e2, Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(5, 5, 5)), 2);
//    octree.Insert(e3, Aabb::FromCenterExtents(Vec3(50, 0, 0), Vec3(5, 5, 5)), 3);
//    
//    // Query left region
//    Int32 leftCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(-50, 0, 0), Vec3(20, 20, 20)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { leftCount++; }
//    );
//    
//    // Query center region
//    Int32 centerCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(20, 20, 20)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { centerCount++; }
//    );
//    
//    // Query right region
//    Int32 rightCount = 0;
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(50, 0, 0), Vec3(20, 20, 20)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { rightCount++; }
//    );
//    
//    return leftCount == 1 && centerCount == 1 && rightCount == 1;
//}
//
//// ============================================================================
//// Stress Tests
//// ============================================================================
//
//void OctreeTest::StressTestInsert(Int32 _numObjects)
//{
//    std::cout << "\n=== Stress Test: Insert " << _numObjects << " objects ===" << std::endl;
//    
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 10;
//    desc.subdivisionThreshold = 16;
//    
//    Octree<Int32> octree(desc);
//    
//    auto startTime = std::chrono::high_resolution_clock::now();
//    
//    for (Int32 i = 0; i < _numObjects; ++i)
//    {
//        Entity entity = CreateTestEntity();
//        Vec3 center = RandomVec3(-500.f, 500.f);
//        Aabb aabb = CreateRandomAabb(center, Vec3(1, 1, 1), Vec3(10, 10, 10));
//        octree.Insert(entity, aabb, i);
//    }
//    
//    auto endTime = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
//    
//    std::cout << "Insertion completed in " << duration.count() << " ms" << std::endl;
//    std::cout << "Average per object: " << (duration.count() / static_cast<float>(_numObjects)) << " ms" << std::endl;
//    
//    // Query all objects to verify
//    Int32 foundCount = 0;
//    auto queryStart = std::chrono::high_resolution_clock::now();
//    octree.Query(
//        Aabb::FromCenterExtents(Vec3(0, 0, 0), Vec3(2000, 2000, 2000)),
//        [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//        [&](const Entity _e, Int32 _data) { foundCount++; }
//    );
//    auto queryEnd = std::chrono::high_resolution_clock::now();
//    auto queryDuration = std::chrono::duration_cast<std::chrono::milliseconds>(queryEnd - queryStart);
//    
//    std::cout << "Query found " << foundCount << " objects in " << queryDuration.count() << " ms" << std::endl;
//}
//
//void OctreeTest::StressTestInsertAndUpdate(Int32 _numObjects, Int32 _numFrames)
//{
//    std::cout << "\n=== Stress Test: Insert and Update " << _numObjects << " objects over " << _numFrames << " frames ===" << std::endl;
//    
//    OctreeDesc desc;
//    desc.initialBaseExtents = Vec3(100.f);
//    desc.maxDepth = 10;
//    desc.subdivisionThreshold = 16;
//    
//    Octree<Int32> octree(desc);
//    
//    // Create test objects
//    std::vector<TestObject> objects;
//    objects.reserve(_numObjects);
//    
//    for (Int32 i = 0; i < _numObjects; ++i)
//    {
//        TestObject obj;
//        obj.entity = CreateTestEntity();
//        Vec3 center = RandomVec3(-200.f, 200.f);
//        obj.aabb = CreateRandomAabb(center, Vec3(1, 1, 1), Vec3(5, 5, 5));
//        obj.velocity = RandomVec3(-5.f, 5.f);
//        obj.userData = i;
//        
//        objects.push_back(obj);
//        octree.Insert(obj.entity, obj.aabb, obj.userData);
//    }
//    
//    std::cout << "Initial insertion completed" << std::endl;
//    
//    // Simulate frames with updates
//    auto startTime = std::chrono::high_resolution_clock::now();
//    
//    Int64 totalUpdateTime = 0;
//    Int64 totalQueryTime = 0;
//    
//    for (Int32 frame = 0; frame < _numFrames; ++frame)
//    {
//        // Update phase
//        auto updateStart = std::chrono::high_resolution_clock::now();
//        
//        for (TestObject& obj : objects)
//        {
//            // Move object
//            Vec3 center = obj.aabb.Center();
//            center += obj.velocity;
//            
//            // Bounce off boundaries
//            if (center.x < -300.f || center.x > 300.f) obj.velocity.x *= -1.f;
//            if (center.y < -300.f || center.y > 300.f) obj.velocity.y *= -1.f;
//            if (center.z < -300.f || center.z > 300.f) obj.velocity.z *= -1.f;
//            
//            obj.aabb = Aabb::FromCenterExtents(center, obj.aabb.Extents());
//            octree.Update(obj.entity, obj.aabb);
//        }
//        
//        auto updateEnd = std::chrono::high_resolution_clock::now();
//        totalUpdateTime += std::chrono::duration_cast<std::chrono::microseconds>(updateEnd - updateStart).count();
//        
//        // Query phase (simulating culling)
//        auto queryStart = std::chrono::high_resolution_clock::now();
//        
//        Vec3 queryCenter = RandomVec3(-100.f, 100.f);
//        Int32 visibleCount = 0;
//        octree.Query(
//            Aabb::FromCenterExtents(queryCenter, Vec3(150, 150, 150)),
//            [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//            [&](const Entity _e, Int32 _data) { visibleCount++; }
//        );
//        
//        auto queryEnd = std::chrono::high_resolution_clock::now();
//        totalQueryTime += std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
//    }
//    
//    auto endTime = std::chrono::high_resolution_clock::now();
//    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
//    
//    std::cout << "\nTotal time: " << totalDuration.count() << " ms" << std::endl;
//    std::cout << "Average update time per frame: " << (totalUpdateTime / _numFrames / 1000.0) << " ms" << std::endl;
//    std::cout << "Average query time per frame: " << (totalQueryTime / _numFrames / 1000.0) << " ms" << std::endl;
//    std::cout << "Average total time per frame: " << (totalDuration.count() / static_cast<float>(_numFrames)) << " ms" << std::endl;
//}
//
//// ============================================================================
//// Performance Test
//// ============================================================================
//
//void OctreeTest::FrustumCullingPerformanceTest(float _durationSeconds)
//{
//    std::cout << "\n=== Frustum Culling Performance Test (" << _durationSeconds << " seconds) ===" << std::endl;
//    
//    // Test with different object counts
//    Int32 objectCounts[] = { 1000, 5000, 10000 };
//    
//    for (Int32 numObjects : objectCounts)
//    {
//        std::cout << "\n--- Testing with " << numObjects << " objects ---" << std::endl;
//        
//        OctreeDesc desc;
//        desc.initialBaseExtents = Vec3(200.f);
//        desc.maxDepth = 10;
//        desc.subdivisionThreshold = 16;
//        
//        Octree<Int32> octree(desc);
//        
//        // Create test objects
//        std::vector<TestObject> objects;
//        objects.reserve(numObjects);
//        
//        for (Int32 i = 0; i < numObjects; ++i)
//        {
//            TestObject obj;
//            obj.entity = CreateTestEntity();
//            Vec3 center = RandomVec3(-500.f, 500.f);
//            obj.aabb = CreateRandomAabb(center, Vec3(2, 2, 2), Vec3(10, 10, 10));
//            obj.velocity = RandomVec3(-2.f, 2.f);
//            obj.userData = i;
//            
//            objects.push_back(obj);
//            octree.Insert(obj.entity, obj.aabb, obj.userData);
//        }
//        
//        // Simulate game loop
//        auto startTime = std::chrono::high_resolution_clock::now();
//        auto currentTime = startTime;
//        
//        Int32 frameCount = 0;
//        Int64 totalUpdateTime = 0;
//        Int64 totalQueryTime = 0;
//        Int32 totalVisibleObjects = 0;
//        
//        // Frustum position and velocity
//        Vec3 frustumCenter = Vec3(0, 0, 0);
//        Vec3 frustumVelocity = RandomVec3(-10.f, 10.f);
//        
//        while (std::chrono::duration<float>(currentTime - startTime).count() < _durationSeconds)
//        {
//            // Update objects
//            auto updateStart = std::chrono::high_resolution_clock::now();
//            
//            for (TestObject& obj : objects)
//            {
//                // Randomly update some objects (30% chance per frame)
//                if (RandomFloat(0.f, 1.f) < 0.3f)
//                {
//                    Vec3 center = obj.aabb.Center();
//                    center += obj.velocity * 0.1f;  // Small movement
//                    
//                    // Bounce off boundaries
//                    if (center.x < -600.f || center.x > 600.f) obj.velocity.x *= -1.f;
//                    if (center.y < -600.f || center.y > 600.f) obj.velocity.y *= -1.f;
//                    if (center.z < -600.f || center.z > 600.f) obj.velocity.z *= -1.f;
//                    
//                    obj.aabb = Aabb::FromCenterExtents(center, obj.aabb.Extents());
//                    octree.Update(obj.entity, obj.aabb);
//                }
//            }
//            
//            auto updateEnd = std::chrono::high_resolution_clock::now();
//            totalUpdateTime += std::chrono::duration_cast<std::chrono::microseconds>(updateEnd - updateStart).count();
//            
//            // Move frustum (camera)
//            frustumCenter += frustumVelocity * 0.5f;
//            
//            // Bounce frustum off boundaries
//            if (frustumCenter.x < -400.f || frustumCenter.x > 400.f) frustumVelocity.x *= -1.f;
//            if (frustumCenter.y < -400.f || frustumCenter.y > 400.f) frustumVelocity.y *= -1.f;
//            if (frustumCenter.z < -400.f || frustumCenter.z > 400.f) frustumVelocity.z *= -1.f;
//            
//            // Query with frustum (using AABB as a simple frustum approximation)
//            auto queryStart = std::chrono::high_resolution_clock::now();
//            
//            Int32 visibleCount = 0;
//            Aabb frustumVolume = Aabb::FromCenterExtents(frustumCenter, Vec3(200, 200, 200));
//            
//            octree.Query(
//                frustumVolume,
//                [](const auto& _bv, const Aabb& _aabb) { return _bv.Intersects(_aabb); },
//                [&](const Entity _e, Int32 _data) { visibleCount++; }
//            );
//            
//            auto queryEnd = std::chrono::high_resolution_clock::now();
//            totalQueryTime += std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
//            
//            totalVisibleObjects += visibleCount;
//            frameCount++;
//            
//            currentTime = std::chrono::high_resolution_clock::now();
//        }
//        
//        auto endTime = std::chrono::high_resolution_clock::now();
//        float actualDuration = std::chrono::duration<float>(endTime - startTime).count();
//        
//        float avgFPS = frameCount / actualDuration;
//        float avgUpdateTime = totalUpdateTime / static_cast<float>(frameCount) / 1000.0f;  // ms
//        float avgQueryTime = totalQueryTime / static_cast<float>(frameCount) / 1000.0f;    // ms
//        float avgVisibleObjects = totalVisibleObjects / static_cast<float>(frameCount);
//        
//        std::cout << std::fixed << std::setprecision(2);
//        std::cout << "Results:" << std::endl;
//        std::cout << "  Total frames: " << frameCount << std::endl;
//        std::cout << "  Average FPS: " << avgFPS << std::endl;
//        std::cout << "  Average frame time: " << (1000.0f / avgFPS) << " ms" << std::endl;
//        std::cout << "  Average update time: " << avgUpdateTime << " ms" << std::endl;
//        std::cout << "  Average query time: " << avgQueryTime << " ms" << std::endl;
//        std::cout << "  Average visible objects: " << avgVisibleObjects << std::endl;
//    }
//}
//
//// ============================================================================
//// Run All Tests
//// ============================================================================
//
//void OctreeTest::RunAllTests()
//{
//    std::cout << "\n============================================" << std::endl;
//    std::cout << "OCTREE TEST SUITE" << std::endl;
//    std::cout << "============================================\n" << std::endl;
//    
//    // Validity Tests
//    std::cout << "=== Validity Tests ===" << std::endl;
//    PrintTestResult("Basic Insert and Query", TestBasicInsertAndQuery());
//    PrintTestResult("Insert Multiple Objects", TestInsertMultipleObjects());
//    PrintTestResult("Update Object", TestUpdateObject());
//    PrintTestResult("Remove Object", TestRemoveObject());
//    PrintTestResult("Octree Expansion", TestOctreeExpansion());
//    PrintTestResult("Octree Subdivision", TestOctreeSubdivision());
//    PrintTestResult("Octree Shrink To Fit", TestOctreeShrinkToFit()); // todo fix
//    PrintTestResult("Boundary Conditions", TestBoundaryConditions());
//    PrintTestResult("Large Object Handling", TestLargeObjectHandling());
//    PrintTestResult("Query with Different Volumes", TestQueryWithDifferentVolumes());
//    
//    // Stress Tests
//    std::cout << "\n=== Stress Tests ===" << std::endl;
//    StressTestInsert(100);
//    StressTestInsert(1000);
//    StressTestInsert(5000);
//    StressTestInsert(10000);
//    
//    StressTestInsertAndUpdate(100, 100);
//    StressTestInsertAndUpdate(1000, 100);
//    StressTestInsertAndUpdate(5000, 50);
//    StressTestInsertAndUpdate(10000, 20);
//    
//    // Performance Test
//    FrustumCullingPerformanceTest(3.f);
//    
//    std::cout << "\n============================================" << std::endl;
//    std::cout << "ALL TESTS COMPLETED" << std::endl;
//    std::cout << "============================================" << std::endl;
//}
