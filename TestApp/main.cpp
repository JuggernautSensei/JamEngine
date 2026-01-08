#include "pch.h"

#include <filesystem>
#include <iostream>
#include <vector>

#define _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
#define _SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING
#include "cista/serialization.h"

// 파일 I/O를 위해 mmap 지원 헤더 포함
#include "cista/mmap.h"

namespace data = cista::offset;   // 파일 저장을 위해 offset 방식 권장

// 1. 하위 구조체
struct profile
{
    data::string nickname_;
    uint32_t     level_;
};

// 2. 가변 타입 (Variant) - 상태 메시지 혹은 시스템 알림
using message_t = data::variant<data::string, uint64_t>;

// 3. 메인 사용자 구조체
struct user
{
    uint64_t                id_;
    profile                 info_;
    data::vector<message_t> history_;
};

// 4. 전체 데이터베이스
struct database
{
    data::vector<user>                     users_;
    data::hash_map<data::string, uint64_t> name_to_id_;
};

int main()
{
    const std::string filename = "db.bin";

    // --- [STEP 1: 데이터 생성 및 파일 저장] ---
    {
        database db;

        // 데이터 추가
        user u1 {
            101, { "Gamer_A", 50 },
             {}
        };
        u1.history_.emplace_back(data::string { "Joined the server" });
        u1.history_.emplace_back(uint64_t { 20240101 });   // 날짜 예시

        db.users_.emplace_back(std::move(u1));
        db.name_to_id_.emplace(data::string { "Gamer_A" }, 101ull);

        // 파일로 직렬화 (검증을 위한 모드 설정)
        // cista::mode::WITH_CRC: 데이터 무결성 체크
        // cista::mode::WITH_VERSION: 구조체 변경 여부 체크
        auto const mode   = cista::mode::WITH_VERSION;
        auto       buffer = cista::serialize<mode>(db);

        // 파일 쓰기
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        std::cout << "Database saved to " << filename << " (" << buffer.size() << " bytes)\n";
    }

    // --- [STEP 2: 파일에서 읽기 (Zero-copy)] ---
    if (std::filesystem::exists(filename))
    {
        // 파일을 메모리에 매핑 (가장 빠른 방식)
        cista::mmap m { filename.c_str(), cista::mmap::protection::READ };

        auto const mode = cista::mode::WITH_VERSION;

        // 역직렬화 - 데이터 복사 없이 메모리 주소만 연결
        auto const db_ptr = data::deserialize<database, mode>(m);

        std::cout << "\n--- Load Success ---\n";
        for (auto const& u: db_ptr->users_)
        {
            std::cout << "User ID: " << u.id_ << ", Nick: " << u.info_.nickname_ << "\n";
        }

        // Hash Map 사용
        if (auto it = db_ptr->name_to_id_.find(data::string { "Gamer_A" }); it != db_ptr->name_to_id_.end())
        {
            std::cout << "Map Lookup (Gamer_A): ID is " << it->second << "\n";
        }
    }

    return 0;
}