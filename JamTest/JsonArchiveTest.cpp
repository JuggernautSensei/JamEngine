#include "pch.h"
#include "../JamEngine/JsonArchive.h"

// ============================================================
// JsonWriter 테스트
// ============================================================

class JsonWriterTest : public ::testing::Test
{
protected:
    JsonWriter writer;
};

// --- 기본 동작 ---

TEST_F(JsonWriterTest, DefaultConstructor_ProducesEmptyObject)
{
    // 기본 생성자 → 루트가 BeginObject 로 시작
    writer.BeginObject();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.GetValue(), "{}");
}

TEST_F(JsonWriterTest, Stringify_PrettyFlag_ContainsNewline)
{
    writer.BeginObject();
    writer.WriteField("key", static_cast<Int32>(42));
    writer.EndObject();

    auto result = writer.Stringify(eJsonWriteFlags::Pretty);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find('\n').IsSucceeded());
}

TEST_F(JsonWriterTest, Stringify_NewlineAtEnd_EndsWithNewline)
{
    writer.BeginObject();
    writer.EndObject();

    auto result = writer.Stringify(eJsonWriteFlags::NewlineAtEnd);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.GetValue().Back(), '\n');
}

// --- BeginObject / BeginArray : 빈 컨테이너 ---

TEST_F(JsonWriterTest, BeginObject_EmptyObject_ProducesEmptyBraces)
{
    writer.BeginObject();
    writer.AddField("obj");
    writer.BeginObject();
    writer.EndObject();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("\"obj\":{}").IsSucceeded());
}

TEST_F(JsonWriterTest, BeginArray_EmptyArray_ProducesEmptyBrackets)
{
    writer.BeginObject();
    writer.AddField("arr");
    writer.BeginArray();
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("\"arr\":[]").IsSucceeded());
}

TEST_F(JsonWriterTest, BeginArray_NestedEmptyArray)
{
    // 배열 안에 빈 배열: {"matrix":[[]]}
    writer.BeginObject();
    writer.AddField("matrix");
    writer.BeginArray();
    writer.AddElement();
    writer.BeginArray();
    writer.EndArray();   // inner []
    writer.EndArray();   // matrix []
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("[[]]").IsSucceeded());
}

// --- WritePrimitive_ : 기본 타입 ---

TEST_F(JsonWriterTest, Write_NullValue)
{
    writer.BeginObject();
    writer.WriteField("n", nullptr);
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("null").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_BoolTrue)
{
    writer.BeginObject();
    writer.WriteField("b", true);
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("true").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_BoolFalse)
{
    writer.BeginObject();
    writer.WriteField("b", false);
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("false").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_Int32)
{
    writer.BeginObject();
    writer.WriteField("i", static_cast<Int32>(-123));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("-123").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_UInt32)
{
    writer.BeginObject();
    writer.WriteField("u", static_cast<UInt32>(4294967295u));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("4294967295").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_Int64)
{
    writer.BeginObject();
    writer.WriteField("i64", static_cast<Int64>(-9000000000LL));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("-9000000000").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_UInt64)
{
    writer.BeginObject();
    writer.WriteField("u64", static_cast<UInt64>(18000000000ULL));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("18000000000").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_Float)
{
    writer.BeginObject();
    writer.WriteField("f", 3.14f);
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("3.14").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_Double)
{
    writer.BeginObject();
    writer.WriteField("d", 2.718281828);
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("2.718").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_CString)
{
    writer.BeginObject();
    writer.WriteField("s", "hello");
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("hello").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_StdString)
{
    writer.BeginObject();
    writer.WriteField("s", "world");
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("world").IsSucceeded());
}

TEST_F(JsonWriterTest, Write_StdStringView)
{
    writer.BeginObject();
    writer.WriteField("sv", std::string_view("view_str"));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("view_str").IsSucceeded());
}

// --- 중첩 구조 ---

TEST_F(JsonWriterTest, NestedObject)
{
    writer.BeginObject();
    writer.AddField("outer");
    writer.BeginObject();
    writer.WriteField("inner", static_cast<Int32>(99));
    writer.EndObject();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("outer").IsSucceeded());
    EXPECT_TRUE(result.GetValue().Find("inner").IsSucceeded());
    EXPECT_TRUE(result.GetValue().Find("99").IsSucceeded());
}

TEST_F(JsonWriterTest, ArrayWrite)
{
    writer.BeginObject();
    writer.AddField("arr");
    writer.BeginArray();
    for (Int32 i = 1; i <= 3; ++i)
        writer.WriteElement(i);
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("[1,2,3]").IsSucceeded());
}

TEST_F(JsonWriterTest, ArrayOfObjects)
{
    // [{"x":1},{"x":2}]
    writer.BeginObject();
    writer.AddField("items");
    writer.BeginArray();
    for (Int32 i = 1; i <= 2; ++i)
    {
        writer.AddElement();
        writer.BeginObject();
        writer.WriteField("x", i);
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("\"x\":1").IsSucceeded());
    EXPECT_TRUE(result.GetValue().Find("\"x\":2").IsSucceeded());
}

TEST_F(JsonWriterTest, DeepNesting_ObjectInsideArrayInsideObject)
{
    // {"a":[{"b":true}]}
    writer.BeginObject();
    writer.AddField("a");
    writer.BeginArray();
    writer.AddElement();
    writer.BeginObject();
    writer.WriteField("b", true);
    writer.EndObject();
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("\"b\":true").IsSucceeded());
}

TEST_F(JsonWriterTest, ArrayInsideArray)
{
    // {"mat":[[1,2],[3,4]]}
    writer.BeginObject();
    writer.AddField("mat");
    writer.BeginArray();
    for (Int32 row = 0; row < 2; ++row)
    {
        writer.AddElement();
        writer.BeginArray();
        for (Int32 col = 0; col < 2; ++col)
            writer.WriteElement(row * 2 + col + 1);
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("[[1,2],[3,4]]").IsSucceeded());
}

// --- WriteField / WriteElement 단축 메서드 ---

TEST_F(JsonWriterTest, WriteField_Shortcut)
{
    writer.BeginObject();
    writer.WriteField("k", static_cast<Int32>(7));
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("\"k\":7").IsSucceeded());
}

TEST_F(JsonWriterTest, WriteElement_Shortcut)
{
    writer.BeginObject();
    writer.AddField("arr");
    writer.BeginArray();
    writer.WriteElement(static_cast<Int32>(10));
    writer.WriteElement(static_cast<Int32>(20));
    writer.EndArray();
    writer.EndObject();

    auto result = writer.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("[10,20]").IsSucceeded());
}

// --- 이동 ---

TEST_F(JsonWriterTest, MoveConstructor)
{
    writer.BeginObject();
    writer.WriteField("x", static_cast<Int32>(7));
    writer.EndObject();

    JsonWriter moved(std::move(writer));
    auto       result = moved.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("7").IsSucceeded());
}

TEST_F(JsonWriterTest, MoveAssignment)
{
    writer.BeginObject();
    writer.WriteField("y", true);
    writer.EndObject();

    JsonWriter other;
    other = std::move(writer);

    auto result = other.Stringify();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.GetValue().Find("true").IsSucceeded());
}

// ============================================================
// JsonReader 테스트
// ============================================================

class JsonReaderTest : public ::testing::Test
{
protected:
    static constexpr const char* kSimpleJson = R"({
        "name": "Alice",
        "age": 30,
        "score": 98.5,
        "active": true,
        "nothing": null,
        "tags": ["cpp", "json", "test"],
        "address": {
            "city": "Seoul",
            "zip": 12345
        }
    })";
};

// --- Parse ---

TEST_F(JsonReaderTest, Parse_ValidJson_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);
}

TEST_F(JsonReaderTest, Parse_InvalidJson_ReturnsError)
{
    auto reader = JsonReader::Parse("{bad json}");
    EXPECT_FALSE(reader);
}

TEST_F(JsonReaderTest, Parse_EmptyString_ReturnsError)
{
    auto reader = JsonReader::Parse("");
    EXPECT_FALSE(reader);
}

// --- EnterField / Exit ---

TEST_F(JsonReaderTest, EnterField_ExistingKey_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    EXPECT_EQ(reader->EnterField("name"), Error::None);
    reader->Exit();
}

TEST_F(JsonReaderTest, EnterField_MissingKey_ReturnsMissingField)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    EXPECT_EQ(reader->EnterField("nonexistent"), Error::MissingField);
}

TEST_F(JsonReaderTest, EnterField_OnArray_ReturnsTypeMismatch)
{
    auto reader = JsonReader::Parse("[1,2,3]");
    ASSERT_TRUE(reader);

    EXPECT_EQ(reader->EnterField("key"), Error::TypeMismatch);
}

// --- EnterAt / Exit ---

TEST_F(JsonReaderTest, EnterAt_ValidIndex_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("tags"), Error::None);
    EXPECT_EQ(reader->EnterAt(0), Error::None);
    reader->Exit();
    reader->Exit();
}

TEST_F(JsonReaderTest, EnterAt_OutOfBounds_ReturnsOutOfRange)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("tags"), Error::None);
    EXPECT_EQ(reader->EnterAt(100), Error::OutOfRange);
    reader->Exit();
}

TEST_F(JsonReaderTest, EnterAt_OnObject_ReturnsTypeMismatch)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    EXPECT_EQ(reader->EnterAt(0), Error::TypeMismatch);
}

// --- GetSize ---

TEST_F(JsonReaderTest, GetSize_Array_ReturnsCorrectSize)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("tags"), Error::None);
    EXPECT_EQ(reader->GetSize(), 3u);
    reader->Exit();
}

TEST_F(JsonReaderTest, GetSize_Object_ReturnsFieldCount)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("address"), Error::None);
    EXPECT_EQ(reader->GetSize(), 2u);
    reader->Exit();
}

TEST_F(JsonReaderTest, GetSize_EmptyArray_ReturnsZero)
{
    auto reader = JsonReader::Parse(R"({"arr":[]})");
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("arr"), Error::None);
    EXPECT_EQ(reader->GetSize(), 0u);
    reader->Exit();
}

TEST_F(JsonReaderTest, GetSize_EmptyObject_ReturnsZero)
{
    auto reader = JsonReader::Parse(R"({"obj":{}})");
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("obj"), Error::None);
    EXPECT_EQ(reader->GetSize(), 0u);
    reader->Exit();
}

// --- IsNull ---

TEST_F(JsonReaderTest, IsNull_NullValue_ReturnsTrue)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("nothing"), Error::None);
    EXPECT_TRUE(reader->IsNull());
    reader->Exit();
}

TEST_F(JsonReaderTest, IsNull_NonNullValue_ReturnsFalse)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("name"), Error::None);
    EXPECT_FALSE(reader->IsNull());
    reader->Exit();
}

// --- ReadPrimitive_ ---

TEST_F(JsonReaderTest, Read_String_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("name"), Error::None);
    std::string val;
    EXPECT_EQ(reader->Read(val), Error::None);
    EXPECT_EQ(val, "Alice");
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_Int32_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("age"), Error::None);
    Int32 val = 0;
    EXPECT_EQ(reader->Read(val), Error::None);
    EXPECT_EQ(val, 30);
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_Double_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("score"), Error::None);
    double val = 0.0;
    EXPECT_EQ(reader->Read(val), Error::None);
    EXPECT_DOUBLE_EQ(val, 98.5);
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_Bool_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("active"), Error::None);
    bool val = false;
    EXPECT_EQ(reader->Read(val), Error::None);
    EXPECT_TRUE(val);
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_Null_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("nothing"), Error::None);
    std::nullptr_t val = nullptr;
    EXPECT_EQ(reader->Read(val), Error::None);
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_TypeMismatch_ReturnsError)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("name"), Error::None);
    Int32 val = 0;
    EXPECT_EQ(reader->Read(val), Error::TypeMismatch);
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_StringView_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("name"), Error::None);
    std::string_view sv;
    EXPECT_EQ(reader->Read(sv), Error::None);
    EXPECT_EQ(sv, "Alice");
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_CString_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("name"), Error::None);
    const char* val = nullptr;
    EXPECT_EQ(reader->Read(val), Error::None);
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, "Alice");
    reader->Exit();
}

TEST_F(JsonReaderTest, Read_NestedObject_Succeeds)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("address"), Error::None);
    ASSERT_EQ(reader->EnterField("city"), Error::None);
    std::string city;
    EXPECT_EQ(reader->Read(city), Error::None);
    EXPECT_EQ(city, "Seoul");
    reader->Exit();
    reader->Exit();
}

// --- ForEachArray ---

TEST_F(JsonReaderTest, ForEach_Array_VisitsAllElements)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("tags"), Error::None);

    std::vector<std::string> tags;
    Error                    err = reader->ForEachArray(Bind([&](SizeT, ArchiveReader& ar) -> Error {
        std::string s;
        Error       e = ar.Read(s);
        if (!e) tags.push_back(s);
        return e; }));

    EXPECT_EQ(err, Error::None);
    ASSERT_EQ(tags.size(), 3u);
    EXPECT_EQ(tags[0], "cpp");
    EXPECT_EQ(tags[1], "json");
    EXPECT_EQ(tags[2], "test");
    reader->Exit();
}

TEST_F(JsonReaderTest, ForEach_EmptyArray_CallbackNeverInvoked)
{
    auto reader = JsonReader::Parse(R"({"arr":[]})");
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("arr"), Error::None);
    SizeT count = 0;
    Error err   = reader->ForEachArray(Bind([&](SizeT, ArchiveReader&) -> Error {
        ++count;
        return Error::None; }));
    EXPECT_EQ(err, Error::None);
    EXPECT_EQ(count, 0u);
    reader->Exit();
}

TEST_F(JsonReaderTest, ForEach_OnObject_ReturnsTypeMismatch)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    Error err = reader->ForEachArray(Bind([](SizeT, ArchiveReader&) -> Error { return Error::None; }));
    EXPECT_EQ(err, Error::TypeMismatch);
}

// --- ForEachObject ---

TEST_F(JsonReaderTest, ForEachField_Object_VisitsAllFields)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("address"), Error::None);

    std::vector<std::string> keys;
    reader->ForEachObject(Bind([&](StringView key, ArchiveReader&) -> Error {
        keys.emplace_back(key.GetPtr(), key.GetLength());
        return Error::None; }));

    ASSERT_EQ(keys.size(), 2u);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "city") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "zip") != keys.end());
    reader->Exit();
}

TEST_F(JsonReaderTest, ForEachField_EmptyObject_CallbackNeverInvoked)
{
    auto reader = JsonReader::Parse(R"({"obj":{}})");
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("obj"), Error::None);
    SizeT count = 0;
    Error err   = reader->ForEachObject(Bind([&](StringView, ArchiveReader&) -> Error {
        ++count;
        return Error::None; }));
    EXPECT_EQ(err, Error::None);
    EXPECT_EQ(count, 0u);
    reader->Exit();
}

TEST_F(JsonReaderTest, ForEachField_OnArray_ReturnsTypeMismatch)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("tags"), Error::None);
    Error err = reader->ForEachObject(Bind([](StringView, ArchiveReader&) -> Error { return Error::None; }));
    EXPECT_EQ(err, Error::TypeMismatch);
    reader->Exit();
}

// --- 에러 상태 관리 ---

TEST_F(JsonReaderTest, ErrorState_InitiallyFine)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);
    EXPECT_TRUE(reader->IsFine());
    EXPECT_FALSE(reader->HasError());
}

TEST_F(JsonReaderTest, ErrorState_MarkError_SetsProperly)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    reader->MarkError(Error::TypeMismatch);
    EXPECT_FALSE(reader->IsFine());
    EXPECT_TRUE(reader->HasError());
    EXPECT_EQ(reader->GetLastError(), Error::TypeMismatch);
}

TEST_F(JsonReaderTest, ErrorState_ClearLastError_ResetsState)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    reader->MarkError(Error::MissingField);
    reader->ClearLastError();
    EXPECT_TRUE(reader->IsFine());
    EXPECT_FALSE(reader->HasError());
}

// --- 이동 ---

TEST_F(JsonReaderTest, MoveConstructor_PreservesData)
{
    auto reader = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(reader);

    JsonReader moved(std::move(*reader));
    ASSERT_EQ(moved.EnterField("name"), Error::None);
    std::string name;
    EXPECT_EQ(moved.Read(name), Error::None);
    EXPECT_EQ(name, "Alice");
    moved.Exit();
}

TEST_F(JsonReaderTest, MoveAssignment_PreservesData)
{
    auto src = JsonReader::Parse(kSimpleJson);
    ASSERT_TRUE(src);

    auto dst = JsonReader::Parse("{}");
    ASSERT_TRUE(dst);

    *dst = std::move(*src);

    ASSERT_EQ(dst->EnterField("age"), Error::None);
    Int32 age = 0;
    EXPECT_EQ(dst->Read(age), Error::None);
    EXPECT_EQ(age, 30);
    dst->Exit();
}

// ============================================================
// Writer -> Reader 왕복(round-trip) 테스트
// ============================================================

class JsonRoundTripTest : public ::testing::Test
{};

TEST_F(JsonRoundTripTest, RoundTrip_PrimitiveFields)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.WriteField("name", "Bob");
    writer.WriteField("age", static_cast<Int32>(25));
    writer.WriteField("score", 77.5);
    writer.WriteField("pass", true);
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    {
        ASSERT_EQ(reader->EnterField("name"), Error::None);
        std::string name;
        EXPECT_EQ(reader->Read(name), Error::None);
        EXPECT_EQ(name, "Bob");
        reader->Exit();
    }
    {
        ASSERT_EQ(reader->EnterField("age"), Error::None);
        Int32 age = 0;
        EXPECT_EQ(reader->Read(age), Error::None);
        EXPECT_EQ(age, 25);
        reader->Exit();
    }
    {
        ASSERT_EQ(reader->EnterField("score"), Error::None);
        double score = 0.0;
        EXPECT_EQ(reader->Read(score), Error::None);
        EXPECT_DOUBLE_EQ(score, 77.5);
        reader->Exit();
    }
    {
        ASSERT_EQ(reader->EnterField("pass"), Error::None);
        bool pass = false;
        EXPECT_EQ(reader->Read(pass), Error::None);
        EXPECT_TRUE(pass);
        reader->Exit();
    }
}

TEST_F(JsonRoundTripTest, RoundTrip_Array)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("nums");
    writer.BeginArray();
    for (Int32 i = 0; i < 5; ++i)
        writer.WriteElement(i);
    writer.EndArray();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("nums"), Error::None);
    EXPECT_EQ(reader->GetSize(), 5u);

    SizeT visitCount = 0;
    reader->ForEachArray(Bind([&](SizeT idx, ArchiveReader& ar) -> Error {
        Int32 v = -1;
        Error e = ar.Read(v);
        EXPECT_EQ(v, static_cast<Int32>(idx));
        ++visitCount;
        return e; }));
    EXPECT_EQ(visitCount, 5u);
    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_EmptyArray)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("empty");
    writer.BeginArray();
    writer.EndArray();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("empty"), Error::None);
    EXPECT_EQ(reader->GetSize(), 0u);

    SizeT count = 0;
    reader->ForEachArray(Bind([&](SizeT, ArchiveReader&) -> Error { ++count; return Error::None; }));
    EXPECT_EQ(count, 0u);
    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_EmptyObject)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("empty");
    writer.BeginObject();
    writer.EndObject();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("empty"), Error::None);
    EXPECT_EQ(reader->GetSize(), 0u);
    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_NestedObject)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("meta");
    writer.BeginObject();
    writer.WriteField("version", static_cast<UInt32>(3u));
    writer.WriteField("label", "release");
    writer.EndObject();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("meta"), Error::None);

    UInt32 ver = 0;
    ASSERT_EQ(reader->EnterField("version"), Error::None);
    EXPECT_EQ(reader->Read(ver), Error::None);
    EXPECT_EQ(ver, 3u);
    reader->Exit();

    std::string label;
    ASSERT_EQ(reader->EnterField("label"), Error::None);
    EXPECT_EQ(reader->Read(label), Error::None);
    EXPECT_EQ(label, "release");
    reader->Exit();

    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_ArrayOfObjects)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("items");
    writer.BeginArray();
    for (Int32 i = 0; i < 3; ++i)
    {
        writer.AddElement();
        writer.BeginObject();
        writer.WriteField("id", i);
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("items"), Error::None);
    EXPECT_EQ(reader->GetSize(), 3u);

    reader->ForEachArray(Bind([](SizeT idx, ArchiveReader& ar) -> Error {
        Int32 id = -1;
        EXPECT_EQ(ar.ReadField("id", id), Error::None);
        EXPECT_EQ(id, static_cast<Int32>(idx));
        return Error::None; }));

    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_NullValue)
{
    JsonWriter writer;
    writer.BeginObject();
    writer.WriteField("empty", nullptr);
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("empty"), Error::None);
    EXPECT_TRUE(reader->IsNull());
    reader->Exit();
}

TEST_F(JsonRoundTripTest, RoundTrip_ArrayInsideArray)
{
    // {"mat":[[0,1],[2,3]]}
    JsonWriter writer;
    writer.BeginObject();
    writer.AddField("mat");
    writer.BeginArray();
    for (Int32 row = 0; row < 2; ++row)
    {
        writer.AddElement();
        writer.BeginArray();
        for (Int32 col = 0; col < 2; ++col)
            writer.WriteElement(row * 2 + col);
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();

    auto strResult = writer.Stringify();
    ASSERT_TRUE(strResult);

    auto reader = JsonReader::Parse(strResult.GetValue());
    ASSERT_TRUE(reader);

    ASSERT_EQ(reader->EnterField("mat"), Error::None);
    EXPECT_EQ(reader->GetSize(), 2u);

    reader->ForEachArray(Bind([](SizeT row, ArchiveReader& rowAr) -> Error { return rowAr.ForEachArray(Bind([row](SizeT col, ArchiveReader& ar) -> Error {
            Int32 v = -1;
            EXPECT_EQ(ar.Read(v), Error::None);
            EXPECT_EQ(v, static_cast<Int32>(row * 2 + col));
            return Error::None; })); }));

    reader->Exit();
}

// ============================================================
// SaveToFile / ParseFromFile 테스트
// ============================================================

class JsonFileTest : public ::testing::Test
{
protected:
    const StringView kTestPath = "/tmp/jam_json_test.json";

    void TearDown() override
    {
        std::remove(kTestPath.GetPtr());
    }
};

TEST_F(JsonFileTest, SaveToFile_And_ParseFromFile_RoundTrip)
{
    {
        JsonWriter writer;
        writer.BeginObject();
        writer.WriteField("val", static_cast<Int32>(42));
        writer.EndObject();

        Error err = writer.SaveToFile(kTestPath);
        EXPECT_EQ(err, Error::None);
    }
    {
        auto reader = JsonReader::ParseFromFile(kTestPath);
        ASSERT_TRUE(reader);

        ASSERT_EQ(reader->EnterField("val"), Error::None);
        Int32 v = 0;
        EXPECT_EQ(reader->Read(v), Error::None);
        EXPECT_EQ(v, 42);
        reader->Exit();
    }
}

TEST_F(JsonFileTest, ParseFromFile_NonExistentFile_ReturnsError)
{
    auto reader = JsonReader::ParseFromFile("/tmp/does_not_exist_12345.json");
    EXPECT_FALSE(reader);
}
