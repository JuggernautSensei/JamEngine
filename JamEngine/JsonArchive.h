#pragma once
#include <yyjson.h>

#include "Archive.h"
#include "TinyVector.h"
#include "File.h"

namespace Jam
{

using JsonValueStack = Tiny::Vector<yyjson_mut_val*>;

enum class eJsonWriteFlags : UInt32
{
    None                = 0,        // 기본값 (Minify, 유효하지 않은 UTF-8/숫자 에러 처리)
    Pretty              = 1 << 0,   // 4칸 들여쓰기 적용
    EscapeUnicode       = 1 << 1,   // 모든 유니코드를 \uXXXX 형태로 이스케이프
    EscapeSlashes       = 1 << 2,   // / 문자를 \/로 이스케이프
    AllowInfAndNan      = 1 << 3,   // Infinity, NaN 숫자를 문자열로 허용
    InfAndNanAsNull     = 1 << 4,   // Infinity, NaN 숫자를 null로 기록
    AllowInvalidUnicode = 1 << 5,   // 잘못된 유니코드를 무시하고 바이트 단위 복사
    PrettyTwoSpaces     = 1 << 6,   // 2칸 들여쓰기 적용
    NewlineAtEnd        = 1 << 7    // JSON 출력 끝에 개행 문자(\n) 추가
};

class JsonWriter : public ArchiveWriter
{
public:
    JsonWriter();
    explicit JsonWriter(yyjson_mut_doc* _pDoc);
    ~JsonWriter() override;

    JsonWriter(const JsonWriter&)            = delete;
    JsonWriter& operator=(const JsonWriter&) = delete;
    JsonWriter(JsonWriter&& _other) noexcept;
    JsonWriter& operator=(JsonWriter&& _other) noexcept;

    [[nodiscard]] Expect<String> Stringify(Flags<eJsonWriteFlags> _flags = {}) const;
    Error                        SaveToFile(StringView _path, Flags<eJsonWriteFlags> _flags = eJsonWriteFlags::None, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const;

    void OpenField(StringView _key) override;
    void OpenAtBack() override;
    void Close() override;

protected:
    void WritePrimitive_(std::nullptr_t) override;
    void WritePrimitive_(bool _value) override;
    void WritePrimitive_(Int32 _value) override;
    void WritePrimitive_(UInt32 _value) override;
    void WritePrimitive_(Int64 _value) override;
    void WritePrimitive_(UInt64 _value) override;
    void WritePrimitive_(float _value) override;
    void WritePrimitive_(double _value) override;
    void WritePrimitive_(const char* _value) override;
    void WritePrimitive_(const std::string& _value) override;
    void WritePrimitive_(std::string_view _value) override;
    void WritePrimitive_(const String& _value) override;
    void WritePrimitive_(StringView _value) override;

private:
    yyjson_mut_doc* m_pDoc       = nullptr;
    JsonValueStack  m_valueStack = {};
    bool            m_ownsDoc    = true;
};

enum class eJsonReadFlags : UInt32
{
    None                 = 0,         // 기본값 (RFC 8259 준수, 엄격한 파싱)
    Insitu               = 1 << 0,    // 입력 데이터를 직접 수정하여 파싱 속도 향상
    StopWhenDone         = 1 << 1,    // JSON 문서 이후 추가 내용이 있어도 에러 없이 중단
    AllowTrailingCommas  = 1 << 2,    // 객체나 배열 끝의 후행 콤마 허용
    AllowComments        = 1 << 3,    // C 스타일 주석 허용
    AllowInfAndNan       = 1 << 4,    // Infinity, NaN 숫자 및 리터럴 허용
    NumberAsRaw          = 1 << 5,    // 모든 숫자를 raw string으로 읽기
    AllowInvalidUnicode  = 1 << 6,    // 잘못된 유니코드를 허용 (보안 주의 필요)
    BigNumAsRaw          = 1 << 7,    // int64/uint64 범위를 넘는 숫자를 raw string으로 읽기
    AllowBom             = 1 << 8,    // UTF-8 BOM을 허용하고 건너뜀
    AllowExtNumber       = 1 << 9,    // 16진수, 앞뒤 소수점 등 확장 숫자 형식 허용
    AllowExtEscape       = 1 << 10,   // \a, \xNN 등 확장 이스케이프 시퀀스 허용
    AllowExtWhitespace   = 1 << 11,   // \v, \f 및 유니코드 공백 문자 허용
    AllowSingleQuotedStr = 1 << 12,   // 작은따옴표(') 문자열 허용
    AllowUnquotedKey     = 1 << 13,   // 따옴표 없는 객체 키 허용
};

using JsonReadValueStack = Tiny::Vector<yyjson_val*>;

class JsonReader : public ArchiveReader
{
public:
    ~JsonReader() override;
    JsonReader(const JsonReader&)            = delete;
    JsonReader& operator=(const JsonReader&) = delete;
    JsonReader(JsonReader&& _other) noexcept;
    JsonReader& operator=(JsonReader&& _other) noexcept;

    explicit JsonReader(yyjson_doc* _pDoc);
    [[nodiscard]] static Expect<JsonReader> Parse(StringView _json, Flags<eJsonReadFlags> _flags = {});
    [[nodiscard]] static Expect<JsonReader> ParseFromFile(StringView _path, Flags<eJsonReadFlags> _flags = {});

    Error EnterField(StringView _key) override;
    Error EnterAt(SizeT _idx) override;
    void  Exit() override;
    SizeT GetSize() const override;
    bool  IsNull() const override;

    template<typename TFunc>
    Error ArrayForEach(TFunc&& _func)
    {
        yyjson_val* pCur = m_valueStack.Back();
        if (!yyjson_is_arr(pCur))
        {
            JAM_LOG_ERROR("YYJson array iteration error: data type not matched (expected array type).");
            MarkError(Error::TypeMismatch);
            return Error::TypeMismatch;
        }

        SizeT           idx  = 0;
        yyjson_val*     pVal = nullptr;
        yyjson_arr_iter iter = {};
        yyjson_arr_iter_init(pCur, &iter);
        while ((pVal = yyjson_arr_iter_next(&iter)))
        {
            m_valueStack.PushBack(pVal);
            const Error err = _func(idx++, *this);
            m_valueStack.PopBack();

            if (err)
            {
                return err;
            }
        }

        return Error::None;
    }

    [[nodiscard]] bool  IsFine() const override;
    void                MarkError(Error _err) override;
    [[nodiscard]] Error GetLastError() const override;
    void                ClearLastError() override;
    [[nodiscard]] bool  HasError() const override;

protected:
    Error ReadPrimitive_(std::nullptr_t&) override;
    Error ReadPrimitive_(bool& _value) override;
    Error ReadPrimitive_(Int32& _value) override;
    Error ReadPrimitive_(UInt32& _value) override;
    Error ReadPrimitive_(Int64& _value) override;
    Error ReadPrimitive_(UInt64& _value) override;
    Error ReadPrimitive_(float& _value) override;
    Error ReadPrimitive_(double& _value) override;
    Error ReadPrimitive_(const char*& _value) override;
    Error ReadPrimitive_(std::string& _value) override;
    Error ReadPrimitive_(std::string_view& _value) override;
    Error ReadPrimitive_(String& _value) override;
    Error ReadPrimitive_(StringView& _value) override;

private:
    yyjson_doc*        m_doc        = nullptr;
    JsonReadValueStack m_valueStack = {};
    Error              m_lastError  = Error::None;
};

}   // namespace Jam