#include "QuickSort.h"
#include "RadixSort.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <entt/core/algorithm.hpp>
#include <entt/entity/entity.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

struct Mesh
{
    char dummy[16];
};
struct Material
{
    char dummy[16];
};
struct Mat4
{
    float m[16];
};
struct DrawItem
{
    static DrawItem Create()
    {
        DrawItem item;
        item.sortKey = 0;
        item.sortKey |= (rand() % std::numeric_limits<uint16_t>::max());
        item.sortKey <<= 32;
        item.sortKey |= (rand() % std::numeric_limits<uint16_t>::max());
        item.sortKey <<= 16;
        item.sortKey |= (rand() % std::numeric_limits<uint16_t>::max());
        return item;
    }
    Mesh*     pMesh     = nullptr;
    Material* pMaterial = nullptr;
    Mat4      world     = {};
    uint64_t  sortKey   = 0;
};

// entt::radix_sort용 getter 함수
struct DrawItemKeyGetter
{
    uint64_t operator()(const DrawItem* item) const
    {
        return item->sortKey;
    }
};

bool IsSorted(const std::vector<DrawItem*>& items)
{
    for (size_t i = 1; i < items.size(); ++i)
    {
        if (items[i]->sortKey < items[i - 1]->sortKey)
        {
            return false;
        }
    }
    return true;
}

bool IsSorted(const std::vector<uint64_t>& keys)
{
    for (size_t i = 1; i < keys.size(); ++i)
    {
        if (keys[i] < keys[i - 1])
        {
            return false;
        }
    }
    return true;
}

int main()
{
    srand(time(NULL));

    std::cout << "Radix Sort Performance Comparison: JAM vs ENTT\n";
    std::cout << "===============================================\n\n";
    std::cout << std::setw(8) << "Items"
              << std::setw(18) << "JAM Radix (μs)"
              << std::setw(18) << "ENTT Radix (μs)"
              << std::setw(12) << "Speedup"
              << std::setw(10) << "Status" << "\n";
    std::cout << std::string(66, '-') << "\n";

    // ENTT radix sort 설정 (11비트씩 처리, 64비트 전체)
    entt::radix_sort<11, 66> entt_radix_sorter;

    int startIterator = 100;
    int endIterator   = 1000000;
    for (int n = startIterator; n <= endIterator; n *= 2)
    {
        // 동일한 데이터 생성
        std::vector<DrawItem> items;
        items.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            items.emplace_back(DrawItem::Create());
        }

        // ===== JAM Radix Sort 테스트 =====
        std::vector<uint64_t>  keys(n);
        std::vector<uint64_t>  tempKeys(n);
        std::vector<DrawItem*> values(n);
        std::vector<DrawItem*> tempValues(n);

        for (int i = 0; i < n; ++i)
        {
            keys[i]   = items[i].sortKey;
            values[i] = &items[i];
        }

        auto jamStart = std::chrono::high_resolution_clock::now();
        jam::RadixSort(keys.data(), tempKeys.data(), values.data(), tempValues.data(), static_cast<uint32_t>(n));
        auto jamEnd      = std::chrono::high_resolution_clock::now();
        auto jamDuration = std::chrono::duration_cast<std::chrono::microseconds>(jamEnd - jamStart);
        bool jamSorted   = IsSorted(keys) && IsSorted(values);

        // ===== ENTT Radix Sort 테스트 =====
        std::vector<DrawItem*> enttItems(n);
        for (int i = 0; i < n; ++i)
        {
            enttItems[i] = &items[i];
        }

        auto enttStart = std::chrono::high_resolution_clock::now();
        entt_radix_sorter(enttItems.begin(), enttItems.end(), DrawItemKeyGetter {});
        auto enttEnd      = std::chrono::high_resolution_clock::now();
        auto enttDuration = std::chrono::duration_cast<std::chrono::microseconds>(enttEnd - enttStart);
        bool enttSorted   = IsSorted(enttItems);

        // 성능 비교 결과
        double speedup = (jamDuration.count() > 0) ?
                             static_cast<double>(enttDuration.count()) / static_cast<double>(jamDuration.count()) :
                             0.0;

        std::string status = (jamSorted && enttSorted) ? "PASS" : "FAIL";
        if (!jamSorted) status += " (JAM✗)";
        if (!enttSorted) status += " (ENTT✗)";

        std::cout << std::setw(8) << n
                  << std::setw(15) << jamDuration.count() << " μs"
                  << std::setw(15) << enttDuration.count() << " μs"
                  << std::setw(9) << std::fixed << std::setprecision(2) << speedup << "x"
                  << std::setw(12) << status << "\n";
    }

    std::cout << "\n";
    std::cout << "Performance Analysis:\n";
    std::cout << "====================\n";
    std::cout << "Speedup > 1.0: JAM RadixSort is faster\n";
    std::cout << "Speedup < 1.0: ENTT RadixSort is faster\n\n";

    std::cout << "Implementation Details:\n";
    std::cout << "- JAM RadixSort:  11-bit buckets, 6 passes for 64-bit keys\n";
    std::cout << "- ENTT RadixSort: 11-bit buckets, 6 passes for 66-bit range\n";
    std::cout << "- Both use LSD (Least Significant Digit) approach\n";
    std::cout << "- ENTT uses template-based compile-time optimization\n";

    return 0;
}