#pragma once
#include <yyjson.h>

#include "Archive.h"
#include "File.h"

namespace Jam
{

enum class eJsonWriteFlags : UInt32
{
    None                = 0,
    Pretty              = 1 << 0,
    EscapeUnicode       = 1 << 1,
    EscapeSlashes       = 1 << 2,
    AllowInfAndNan      = 1 << 3,
    InfAndNanAsNull     = 1 << 4,
    AllowInvalidUnicode = 1 << 5,
    PrettyTwoSpaces     = 1 << 6,
    NewlineAtEnd        = 1 << 7,
};

class JsonWriter final : public ArchiveWriter
{
    struct Frame
    {
        yyjson_mut_val* node = nullptr;
        eMode           mode = eMode::None;
    };
    static constexpr Int32 kMaxDepth = 64;

public:
    JsonWriter();
    explicit JsonWriter(const yyjson_alc* _alc);
    explicit JsonWriter(yyjson_mut_doc* _pDoc);
    ~JsonWriter() override;

    JsonWriter(const JsonWriter&)            = delete;
    JsonWriter& operator=(const JsonWriter&) = delete;
    JsonWriter(JsonWriter&& _other) noexcept;
    JsonWriter& operator=(JsonWriter&& _other) noexcept;

    [[nodiscard]] Expect<String> Stringify(Flags<eJsonWriteFlags> _flags = {}) const;
    Error                        SaveToFile(StringView _path, Flags<eJsonWriteFlags> _flags = eJsonWriteFlags::None, eFileWrite _mode = eFileWrite::Truncate) const;

    void BeginObject() override;
    void EndObject() override;
    void BeginArray() override;
    void EndArray() override;

    void AddField(StringView _key) override;
    void AddElement() override;

private:
    void WritePrimitive_(std::nullptr_t) override;
    void WritePrimitive_(bool _value) override;
    void WritePrimitive_(Int32 _value) override;
    void WritePrimitive_(UInt32 _value) override;
    void WritePrimitive_(Int64 _value) override;
    void WritePrimitive_(UInt64 _value) override;
    void WritePrimitive_(float _value) override;
    void WritePrimitive_(double _value) override;
    void WritePrimitive_(StringView _value) override;

    void                 Attach_(yyjson_mut_val* _val);
    [[nodiscard]] Frame& Top_();
    [[nodiscard]] Frame  Top_() const;
    void                 Push_(yyjson_mut_val* _node, eMode _mode);
    void                 Pop_();

    yyjson_mut_doc*          m_pDoc       = nullptr;
    bool                     m_ownsDoc    = true;
    StringView               m_pendingKey = {};
    Array<Frame, kMaxDepth> m_stack      = {};
    Int32                    m_stackTop   = -1;
};

enum class eJsonReadFlags : UInt32
{
    None                 = 0,
    Insitu               = 1 << 0,
    StopWhenDone         = 1 << 1,
    AllowTrailingCommas  = 1 << 2,
    AllowComments        = 1 << 3,
    AllowInfAndNan       = 1 << 4,
    NumberAsRaw          = 1 << 5,
    AllowInvalidUnicode  = 1 << 6,
    BigNumAsRaw          = 1 << 7,
    AllowBom             = 1 << 8,
    AllowExtNumber       = 1 << 9,
    AllowExtEscape       = 1 << 10,
    AllowExtWhitespace   = 1 << 11,
    AllowSingleQuotedStr = 1 << 12,
    AllowUnquotedKey     = 1 << 13,
};

class JsonReader final : public ArchiveReader
{
public:
    explicit JsonReader(yyjson_doc* _pDoc);
    ~JsonReader() override;

    JsonReader(const JsonReader&)            = delete;
    JsonReader& operator=(const JsonReader&) = delete;
    JsonReader(JsonReader&& _other) noexcept;
    JsonReader& operator=(JsonReader&& _other) noexcept;

    [[nodiscard]] static Expect<JsonReader> Parse(StringView _json, Flags<eJsonReadFlags> _flags = {});
    [[nodiscard]] static Expect<JsonReader> ParseFromFile(StringView _path, Flags<eJsonReadFlags> _flags = {});

    Error EnterField(StringView _key) override;
    Error EnterAt(SizeT _index) override;
    void  Exit() override;

    SizeT GetSize() const override;
    bool  IsNull() const override;

    Error ForEachArray(const Delegate<Error(SizeT, ArchiveReader&)>& _func) override;
    Error ForEachObject(const Delegate<Error(StringView, ArchiveReader&)>& _func) override;

    bool  IsFine() const override;
    void  MarkError(Error _err) override;
    Error GetLastError() const override;
    bool  HasError() const override;
    void  ClearLastError() override;

protected:
    Error ReadPrimitive_(std::nullptr_t&) override;
    Error ReadPrimitive_(bool& _value) override;
    Error ReadPrimitive_(Int32& _value) override;
    Error ReadPrimitive_(UInt32& _value) override;
    Error ReadPrimitive_(Int64& _value) override;
    Error ReadPrimitive_(UInt64& _value) override;
    Error ReadPrimitive_(float& _value) override;
    Error ReadPrimitive_(double& _value) override;
    Error ReadPrimitive_(String& _value) override;
    Error ReadPrimitive_(StringView& _value) override;

private:
    static constexpr Int32 kMaxDepth = 64;

    [[nodiscard]] yyjson_val* Top_() const;
    void                      Push_(yyjson_val* _val);
    void                      Pop_();

    yyjson_doc*                    m_doc       = nullptr;
    Array<yyjson_val*, kMaxDepth> m_stack     = {};
    Int32                          m_stackTop  = -1;
    Error                          m_lastError = Error::None;
};

}   // namespace Jam