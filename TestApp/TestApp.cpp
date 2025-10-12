#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <new>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

template<typename T>
struct ControlBlock
{
    int strong = 1;   // non-thread-safe
    alignas(T) unsigned char storage[sizeof(T)];

    T*       ptr() noexcept { return reinterpret_cast<T*>(storage); }
    const T* ptr() const noexcept { return reinterpret_cast<const T*>(storage); }
};

template<typename T>
class SharedPtr
{
public:
    SharedPtr() noexcept = default;
    explicit SharedPtr(ControlBlock<T>* cb) noexcept
        : cb_(cb)
    {
    }

    SharedPtr(const SharedPtr& rhs) noexcept
        : cb_(rhs.cb_)
    {
        retain_();
    }
    SharedPtr(SharedPtr&& rhs) noexcept
        : cb_(rhs.cb_)
    {
        rhs.cb_ = nullptr;
    }

    template<class U>
    SharedPtr(const SharedPtr<U>& rhs) noexcept
        requires (std::is_convertible_v<U*, T*>)
        : cb_(reinterpret_cast<ControlBlock<T>*>(rhs.cb_))
    {
        retain_();
    }

    ~SharedPtr() { release_(); }

    SharedPtr& operator=(const SharedPtr& rhs) noexcept
    {
        if (this != &rhs)
        {
            release_();
            cb_ = rhs.cb_;
            retain_();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& rhs) noexcept
    {
        if (this != &rhs)
        {
            release_();
            cb_     = rhs.cb_;
            rhs.cb_ = nullptr;
        }
        return *this;
    }

    T* get() const noexcept
    {
        return cb_ ? cb_->ptr() : nullptr;
    }
    T& operator*() const
    {
        return *cb_->ptr();
    }
    T* operator->() const
    {
        return cb_->ptr();
    }
    int      use_count() const noexcept { return cb_ ? cb_->strong : 0; }
    explicit operator bool() const noexcept { return get() != nullptr; }

    void reset() noexcept { release_(); }
    void swap(SharedPtr& other) noexcept { std::swap(cb_, other.cb_); }

    template<class... Args>
    void reset_construct(Args&&... args)
    {
        release_();
        void* mem = ::operator new(sizeof(ControlBlock<T>), static_cast<std::align_val_t>(alignof(ControlBlock<T>)));
        auto* cb  = new (mem) ControlBlock<T>();   // strong = 1
        try
        {
            std::construct_at(cb->ptr(), std::forward<Args>(args)...);
        }
        catch (...)
        {
            ::operator delete(mem, static_cast<std::align_val_t>(alignof(ControlBlock<T>)));
            throw;
        }
        cb_ = cb;
    }

private:
    template<class U>
    friend class SharedPtr;

    void retain_() noexcept
    {
        if (cb_) ++cb_->strong;
    }

    void release_() noexcept
    {
        if (!cb_) return;
        assert(cb_->strong > 0);
        if (--cb_->strong == 0)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy_at(cb_->ptr());
            }
            ::operator delete(static_cast<void*>(cb_), std::align_val_t(alignof(ControlBlock<T>)));
        }
        cb_ = nullptr;
    }

    ControlBlock<T>* cb_ = nullptr;
};

template<typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
    void* mem = ::operator new(sizeof(ControlBlock<T>), std::align_val_t(alignof(ControlBlock<T>)));
    auto* cb  = new (mem) ControlBlock<T>();   // strong = 1
    try
    {
        std::construct_at(cb->ptr(), std::forward<Args>(args)...);
    }
    catch (...)
    {
        ::operator delete(mem, std::align_val_t(alignof(ControlBlock<T>)));
        throw;
    }
    return SharedPtr<T>(cb);
}

// 벤치마크용 테스트 클래스
struct TestObject
{
    int data[10];
    TestObject(int val = 42)
    {
        for (int& d: data)
            d = val;
    }
    ~TestObject() = default;
};

// 벤치마크 헬퍼 함수
template<typename Func>
double BenchmarkTime(Func&& func, int iterations = 1000000)
{
    auto start = std::chrono::high_resolution_clock::now();
    func(iterations);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void TestCreationPerformance()
{
    std::cout << "=== 생성 성능 테스트 ===\n";

    // 커스텀 SharedPtr 생성
    auto customTime = BenchmarkTime([](int iterations)
                                    {
        for (int i = 0; i < iterations; ++i) {
            auto ptr = MakeShared<TestObject>(i);
            (void)ptr; // 최적화 방지
        } });

    // std::shared_ptr 생성
    auto stdTime = BenchmarkTime([](int iterations)
                                 {
        for (int i = 0; i < iterations; ++i) {
            auto ptr = std::make_shared<TestObject>(i);
            (void)ptr; // 최적화 방지
        } });

    std::cout << "커스텀 SharedPtr: " << customTime << "ms\n";
    std::cout << "std::shared_ptr: " << stdTime << "ms\n";
    std::cout << "성능 비율: " << (customTime / stdTime) << "x\n\n";
}

void TestCopyPerformance()
{
    std::cout << "=== 복사 성능 테스트 ===\n";

    auto customPtr = MakeShared<TestObject>();
    auto stdPtr    = std::make_shared<TestObject>();

    // 커스텀 SharedPtr 복사
    auto customTime = BenchmarkTime([&](int iterations)
                                    {
        for (int i = 0; i < iterations; ++i) {
            auto copy = customPtr;
            (void)copy; // 최적화 방지
        } });

    // std::shared_ptr 복사
    auto stdTime = BenchmarkTime([&](int iterations)
                                 {
        for (int i = 0; i < iterations; ++i) {
            auto copy = stdPtr;
            (void)copy; // 최적화 방지
        } });

    std::cout << "커스텀 SharedPtr: " << customTime << "ms\n";
    std::cout << "std::shared_ptr: " << stdTime << "ms\n";
    std::cout << "성능 비율: " << (customTime / stdTime) << "x\n\n";
}

void TestAccessPerformance()
{
    std::cout << "=== 접근 성능 테스트 ===\n";

    auto customPtr = MakeShared<TestObject>();
    auto stdPtr    = std::make_shared<TestObject>();

    // 커스텀 SharedPtr 접근
    auto customTime = BenchmarkTime([&](int iterations)
                                    {
        volatile int sum = 0;
        for (int i = 0; i < iterations; ++i) {
            sum += customPtr->data[0];
        } });

    // std::shared_ptr 접근
    auto stdTime = BenchmarkTime([&](int iterations)
                                 {
        volatile int sum = 0;
        for (int i = 0; i < iterations; ++i) {
            sum += stdPtr->data[0];
        } });

    std::cout << "커스텀 SharedPtr: " << customTime << "ms\n";
    std::cout << "std::shared_ptr: " << stdTime << "ms\n";
    std::cout << "성능 비율: " << (customTime / stdTime) << "x\n\n";
}

void TestMemoryUsage()
{
    std::cout << "=== 메모리 사용량 비교 ===\n";

    std::cout << "sizeof(SharedPtr<TestObject>): " << sizeof(SharedPtr<TestObject>) << " bytes\n";
    std::cout << "sizeof(std::shared_ptr<TestObject>): " << sizeof(std::shared_ptr<TestObject>) << " bytes\n";
    std::cout << "sizeof(ControlBlock<TestObject>): " << sizeof(ControlBlock<TestObject>) << " bytes\n";

    // std::shared_ptr의 control block 크기는 구현에 따라 다름
    std::cout << "TestObject 크기: " << sizeof(TestObject) << " bytes\n\n";
}

void TestContainerPerformance()
{
    std::cout << "=== 컨테이너 성능 테스트 ===\n";
    const int container_size = 100000;

    // 커스텀 SharedPtr 벡터
    auto customTime = BenchmarkTime([&](int iterations)
                                    {
        for (int iter = 0; iter < iterations / 100; ++iter) {
            std::vector<SharedPtr<TestObject>> vec;
            vec.reserve(container_size);
            
            auto basePtr = MakeShared<TestObject>();
            for (int i = 0; i < container_size; ++i) {
                vec.push_back(basePtr);
            }
            
            // 벡터 순회
            volatile int sum = 0;
            for (const auto& ptr : vec) {
                sum += ptr->data[0];
            }
        } },
                                    100);

    // std::shared_ptr 벡터
    auto stdTime = BenchmarkTime([&](int iterations)
                                 {
        for (int iter = 0; iter < iterations / 100; ++iter) {
            std::vector<std::shared_ptr<TestObject>> vec;
            vec.reserve(container_size);
            
            auto basePtr = std::make_shared<TestObject>();
            for (int i = 0; i < container_size; ++i) {
                vec.push_back(basePtr);
            }
            
            // 벡터 순회
            volatile int sum = 0;
            for (const auto& ptr : vec) {
                sum += ptr->data[0];
            }
        } },
                                 100);

    std::cout << "커스텀 SharedPtr: " << customTime << "ms\n";
    std::cout << "std::shared_ptr: " << stdTime << "ms\n";
    std::cout << "성능 비율: " << (customTime / stdTime) << "x\n\n";
}

int main()
{
    std::cout << "SharedPtr vs std::shared_ptr 성능 비교\n";
    std::cout << "======================================\n\n";

    TestMemoryUsage();
    TestCreationPerformance();
    TestCopyPerformance();
    TestAccessPerformance();
    TestContainerPerformance();

    std::cout << "=== 분석 결과 ===\n";
    std::cout << "커스텀 SharedPtr의 특징:\n";
    std::cout << "✓ 단일 메모리 할당으로 더 나은 캐시 지역성\n";
    std::cout << "✓ 스레드 안전성 없음으로 인한 오버헤드 감소\n";
    std::cout << "✓ weak_ptr 미지원으로 인한 단순한 구조\n";
    std::cout << "⚠ 스레드 안전성 부족\n";
    std::cout << "⚠ weak_ptr 미지원\n";
    std::cout << "⚠ 표준 라이브러리 호환성 부족\n";

    return 0;
}
