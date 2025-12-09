#pragma once

class OctreeTest
{
public:
    // Validity tests
    static bool TestBasicInsertAndQuery();
    static bool TestInsertMultipleObjects();
    static bool TestUpdateObject();
    static bool TestRemoveObject();
    static bool TestOctreeExpansion();
    static bool TestOctreeSubdivision();
    static bool TestOctreeShrinkToFit();
    static bool TestBoundaryConditions();
    static bool TestLargeObjectHandling();
    static bool TestQueryWithDifferentVolumes();
    
    // Stress tests
    static void StressTestInsert(Int32 _numObjects);
    static void StressTestInsertAndUpdate(Int32 _numObjects, Int32 _numFrames);
    
    // Performance test
    static void FrustumCullingPerformanceTest(float _durationSeconds);
    
    // Run all tests
    static void RunAllTests();
    
private:
    struct TestObject
    {
        Entity entity;
        Aabb   aabb;
        Vec3   velocity;
        Int32  userData;
    };
    
    static void PrintTestResult(const char* _testName, bool _passed);
    static Entity CreateTestEntity();
    static Aabb CreateRandomAabb(Vec3 _center, Vec3 _minExtents, Vec3 _maxExtents);
    static Vec3 RandomVec3(float _min, float _max);
    static float RandomFloat(float _min, float _max);
};
