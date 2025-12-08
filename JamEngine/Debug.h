#pragma once

namespace jam
{

struct SourceLoc
{
    std::string filename;
    std::string function;
    UInt32      line;
    UInt32      col;
};

/// @brief 호출자 정보를 가져옵니다.
/// @param _stackIndex 스택 인덱스
/// @note _stackIndex = 0 : TraceCallerInfo 함수 자신
/// @note _stackIndex = 1 : TraceCallerInfo 함수를 호출한 함수
/// @note _stackIndex = 2 : TraceCallerInfo 함수를 호출한 함수의 호출자 (일반적으로 이 값을 사용)
[[nodiscard]] SourceLoc TraceCallerInfo(Int32 _stackIndex = 2);

}   // namespace jam