#include "pch.h"
#include <yyjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/reader.h>

#include "../JamEngine/JsonArchive.h"   // Jam::JsonWriter, Jam::JsonReader

#include <cstring>
#include <string>
#include <vector>
#include <cmath>

// ============================================================
//  공용 데이터 셋
// ============================================================

static constexpr int k_entityCount = 200;   // 엔티티 수 (조절 가능)

struct EntityData
{
    int         id;
    const char* tag;
    float       pos[3];
    float       rot[4];
    bool        active;
    double      health;
};

struct SceneData
{
    const char*             name;
    int                     version;
    std::vector<EntityData> entities;
};

static SceneData MakeScene()
{
    SceneData scene;
    scene.name    = "BenchmarkScene";
    scene.version = 3;
    scene.entities.reserve(k_entityCount);

    for (int i = 0; i < k_entityCount; ++i)
    {
        EntityData e;
        e.id     = i;
        e.tag    = (i % 3 == 0) ? "enemy" : (i % 3 == 1) ? "player"
                                                         : "npc";
        e.pos[0] = static_cast<float>(i) * 1.1f;
        e.pos[1] = static_cast<float>(i) * 2.2f;
        e.pos[2] = static_cast<float>(i) * 3.3f;
        e.rot[0] = 0.f;
        e.rot[1] = 0.f;
        e.rot[2] = std::sin(static_cast<float>(i) * 0.01f);
        e.rot[3] = std::cos(static_cast<float>(i) * 0.01f);
        e.active = (i % 2 == 0);
        e.health = 100.0 - static_cast<double>(i % 100);
        scene.entities.push_back(e);
    }
    return scene;
}

// 전역 싱글턴 — 초기화 1회
static const SceneData& GetScene()
{
    static SceneData s = MakeScene();
    return s;
}

// 미리 만든 JSON 문자열 (Read 벤치에서 파싱 대상)
static const std::string& GetSceneJson_Jam()
{
    static std::string s = []() -> std::string {
        const SceneData& scene = GetScene();
        Jam::JsonWriter  writer;

        writer.BeginObject();
        writer.WriteField("name", scene.name);
        writer.WriteField("version", static_cast<Jam::Int32>(scene.version));

        writer.AddField("entities");
        writer.BeginArray();
        for (const auto& e: scene.entities)
        {
            writer.AddElement();
            writer.BeginObject();
            writer.WriteField("id", static_cast<Jam::Int32>(e.id));
            writer.WriteField("tag", e.tag);
            writer.WriteField("active", e.active);
            writer.WriteField("health", e.health);

            writer.AddField("position");
            writer.BeginArray();
            writer.WriteElement(e.pos[0]);
            writer.WriteElement(e.pos[1]);
            writer.WriteElement(e.pos[2]);
            writer.EndArray();

            writer.AddField("rotation");
            writer.BeginArray();
            writer.WriteElement(e.rot[0]);
            writer.WriteElement(e.rot[1]);
            writer.WriteElement(e.rot[2]);
            writer.WriteElement(e.rot[3]);
            writer.EndArray();

            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        auto r = writer.Stringify();
        return r ? std::string(r.GetValue().GetPtr(), r.GetValue().GetLength()) : "";
    }();
    return s;
}

// ============================================================
//  ── 1. Jam JsonWriter Write 벤치 ───────────────────────────
// ============================================================

static void BM_Jam_Write(benchmark::State& state)
{
    const SceneData& scene      = GetScene();
    std::size_t      totalBytes = 0;

    for (auto _: state)
    {
        Jam::JsonWriter writer;

        writer.BeginObject();
        writer.WriteField("name", scene.name);
        writer.WriteField("version", static_cast<Jam::Int32>(scene.version));

        writer.AddField("entities");
        writer.BeginArray();
        for (const auto& e: scene.entities)
        {
            writer.AddElement();
            writer.BeginObject();
            writer.WriteField("id", static_cast<Jam::Int32>(e.id));
            writer.WriteField("tag", e.tag);
            writer.WriteField("active", e.active);
            writer.WriteField("health", e.health);

            writer.AddField("position");
            writer.BeginArray();
            writer.WriteElement(e.pos[0]);
            writer.WriteElement(e.pos[1]);
            writer.WriteElement(e.pos[2]);
            writer.EndArray();

            writer.AddField("rotation");
            writer.BeginArray();
            writer.WriteElement(e.rot[0]);
            writer.WriteElement(e.rot[1]);
            writer.WriteElement(e.rot[2]);
            writer.WriteElement(e.rot[3]);
            writer.EndArray();

            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        auto result = writer.Stringify();
        if (result)
            totalBytes += result.GetValue().GetLength();

        benchmark::DoNotOptimize(result);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_Jam_Write)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 2. Jam JsonReader Read 벤치 ────────────────────────────
// ============================================================

static void BM_Jam_Read(benchmark::State& state)
{
    const std::string& json       = GetSceneJson_Jam();
    std::size_t        totalBytes = 0;

    for (auto _: state)
    {
        auto readerExp = Jam::JsonReader::Parse(
            Jam::StringView { json.c_str(), static_cast<Jam::SizeT>(json.size()) });

        if (!readerExp) continue;
        Jam::JsonReader& reader = *readerExp;

        std::string name;
        Jam::Int32  version = 0;
        reader.ReadField("name", name);
        reader.ReadField("version", version);

        reader.EnterField("entities");
        reader.ForEachArray(Bind([](Jam::SizeT, Jam::ArchiveReader& ar) -> Jam::Error {
            Jam::Int32  id     = 0;
            bool        active = false;
            double      health = 0.0;
            std::string tag;

            ar.ReadField("id",     id);
            ar.ReadField("tag",    tag);
            ar.ReadField("active", active);
            ar.ReadField("health", health);

            ar.EnterField("position");
            float px = 0, py = 0, pz = 0;
            ar.ReadAt(0, px);
            ar.ReadAt(1, py);
            ar.ReadAt(2, pz);
            ar.Exit();

            ar.EnterField("rotation");
            float rx = 0, ry = 0, rz = 0, rw = 0;
            ar.ReadAt(0, rx);
            ar.ReadAt(1, ry);
            ar.ReadAt(2, rz);
            ar.ReadAt(3, rw);
            ar.Exit();

            benchmark::DoNotOptimize(id);
            benchmark::DoNotOptimize(health);
            benchmark::DoNotOptimize(px);
            return Jam::Error::None; }));
        reader.Exit();

        totalBytes += json.size();
        benchmark::DoNotOptimize(name);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_Jam_Read)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 3. Jam RoundTrip 벤치 ──────────────────────────────────
// ============================================================

static void BM_Jam_RoundTrip(benchmark::State& state)
{
    const SceneData& scene      = GetScene();
    std::size_t      totalBytes = 0;

    for (auto _: state)
    {
        // Write
        Jam::JsonWriter writer;
        writer.BeginObject();
        writer.WriteField("name", scene.name);
        writer.WriteField("version", static_cast<Jam::Int32>(scene.version));
        writer.AddField("entities");
        writer.BeginArray();
        for (const auto& e: scene.entities)
        {
            writer.AddElement();
            writer.BeginObject();
            writer.WriteField("id", static_cast<Jam::Int32>(e.id));
            writer.WriteField("tag", e.tag);
            writer.WriteField("active", e.active);
            writer.WriteField("health", e.health);
            writer.AddField("position");
            writer.BeginArray();
            writer.WriteElement(e.pos[0]);
            writer.WriteElement(e.pos[1]);
            writer.WriteElement(e.pos[2]);
            writer.EndArray();
            writer.AddField("rotation");
            writer.BeginArray();
            writer.WriteElement(e.rot[0]);
            writer.WriteElement(e.rot[1]);
            writer.WriteElement(e.rot[2]);
            writer.WriteElement(e.rot[3]);
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        auto strResult = writer.Stringify();
        if (!strResult) continue;

        totalBytes += strResult.GetValue().GetLength();

        // Read
        auto readerExp = Jam::JsonReader::Parse(
            Jam::StringView { strResult.GetValue().GetPtr(), strResult.GetValue().GetLength() });
        if (!readerExp) continue;

        benchmark::DoNotOptimize(*readerExp);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_Jam_RoundTrip)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 4. yyjson raw Write 벤치 ───────────────────────────────
//  (Jam JsonWriter 가 올바르게 yyjson 을 쓰는지 확인하는 기준선)
// ============================================================

static void BM_YYJson_Raw_Write(benchmark::State& state)
{
    const SceneData& scene      = GetScene();
    std::size_t      totalBytes = 0;

    for (auto _: state)
    {
        yyjson_mut_doc* doc  = yyjson_mut_doc_new(nullptr);
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);

        yyjson_mut_obj_add_strcpy(doc, root, "name", scene.name);
        yyjson_mut_obj_add_int(doc, root, "version", scene.version);

        yyjson_mut_val* arr = yyjson_mut_obj_add_arr(doc, root, "entities");
        for (const auto& e: scene.entities)
        {
            yyjson_mut_val* obj = yyjson_mut_arr_add_obj(doc, arr);
            yyjson_mut_obj_add_int(doc, obj, "id", e.id);
            yyjson_mut_obj_add_strcpy(doc, obj, "tag", e.tag);
            yyjson_mut_obj_add_bool(doc, obj, "active", e.active);
            yyjson_mut_obj_add_real(doc, obj, "health", e.health);

            yyjson_mut_val* pos = yyjson_mut_obj_add_arr(doc, obj, "position");
            yyjson_mut_arr_add_real(doc, pos, e.pos[0]);
            yyjson_mut_arr_add_real(doc, pos, e.pos[1]);
            yyjson_mut_arr_add_real(doc, pos, e.pos[2]);

            yyjson_mut_val* rot = yyjson_mut_obj_add_arr(doc, obj, "rotation");
            yyjson_mut_arr_add_real(doc, rot, e.rot[0]);
            yyjson_mut_arr_add_real(doc, rot, e.rot[1]);
            yyjson_mut_arr_add_real(doc, rot, e.rot[2]);
            yyjson_mut_arr_add_real(doc, rot, e.rot[3]);
        }

        size_t len  = 0;
        char*  json = yyjson_mut_write(doc, 0, &len);
        if (json)
        {
            totalBytes += len;
            free(json);
        }
        yyjson_mut_doc_free(doc);
        benchmark::DoNotOptimize(len);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_YYJson_Raw_Write)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 5. yyjson raw Read 벤치 ────────────────────────────────
// ============================================================

static void BM_YYJson_Raw_Read(benchmark::State& state)
{
    const std::string& json       = GetSceneJson_Jam();
    std::size_t        totalBytes = 0;

    for (auto _: state)
    {
        yyjson_doc* doc  = yyjson_read(json.c_str(), json.size(), 0);
        yyjson_val* root = yyjson_doc_get_root(doc);

        yyjson_val* name    = yyjson_obj_get(root, "name");
        yyjson_val* version = yyjson_obj_get(root, "version");
        yyjson_val* ents    = yyjson_obj_get(root, "entities");

        size_t      idx = 0, max = 0;
        yyjson_val* ent = nullptr;
        yyjson_arr_foreach(ents, idx, max, ent)
        {
            yyjson_val* id     = yyjson_obj_get(ent, "id");
            yyjson_val* tag    = yyjson_obj_get(ent, "tag");
            yyjson_val* active = yyjson_obj_get(ent, "active");
            yyjson_val* health = yyjson_obj_get(ent, "health");
            yyjson_val* pos    = yyjson_obj_get(ent, "position");
            yyjson_val* rot    = yyjson_obj_get(ent, "rotation");

            int    eid    = static_cast<int>(yyjson_get_sint(id));
            double eheath = yyjson_get_real(health);
            float  px     = static_cast<float>(yyjson_get_real(yyjson_arr_get(pos, 0)));

            benchmark::DoNotOptimize(eid);
            benchmark::DoNotOptimize(eheath);
            benchmark::DoNotOptimize(px);
            (void)tag;
            (void)active;
            (void)rot;
        }

        totalBytes += json.size();
        yyjson_doc_free(doc);
        benchmark::DoNotOptimize(name);
        benchmark::DoNotOptimize(version);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_YYJson_Raw_Read)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 6. RapidJSON Write 벤치 ────────────────────────────────
// ============================================================

static void BM_RapidJson_Write(benchmark::State& state)
{
    const SceneData& scene      = GetScene();
    std::size_t      totalBytes = 0;

    for (auto _: state)
    {
        rapidjson::StringBuffer                    sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();

        writer.Key("name");
        writer.String(scene.name);
        writer.Key("version");
        writer.Int(scene.version);

        writer.Key("entities");
        writer.StartArray();
        for (const auto& e: scene.entities)
        {
            writer.StartObject();
            writer.Key("id");
            writer.Int(e.id);
            writer.Key("tag");
            writer.String(e.tag);
            writer.Key("active");
            writer.Bool(e.active);
            writer.Key("health");
            writer.Double(e.health);

            writer.Key("position");
            writer.StartArray();
            writer.Double(e.pos[0]);
            writer.Double(e.pos[1]);
            writer.Double(e.pos[2]);
            writer.EndArray();

            writer.Key("rotation");
            writer.StartArray();
            writer.Double(e.rot[0]);
            writer.Double(e.rot[1]);
            writer.Double(e.rot[2]);
            writer.Double(e.rot[3]);
            writer.EndArray();

            writer.EndObject();
        }
        writer.EndArray();

        writer.EndObject();

        totalBytes += sb.GetSize();
        benchmark::DoNotOptimize(sb.GetString());
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_RapidJson_Write)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 7. RapidJSON Read 벤치 ─────────────────────────────────
// ============================================================

static void BM_RapidJson_Read(benchmark::State& state)
{
    const std::string& json       = GetSceneJson_Jam();
    std::size_t        totalBytes = 0;

    for (auto _: state)
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());

        const auto& name    = doc["name"].GetString();
        const auto  version = doc["version"].GetInt();
        const auto& ents    = doc["entities"].GetArray();

        for (const auto& ent: ents)
        {
            int         eid    = ent["id"].GetInt();
            const char* tag    = ent["tag"].GetString();
            bool        active = ent["active"].GetBool();
            double      health = ent["health"].GetDouble();
            float       px     = static_cast<float>(ent["position"][0].GetDouble());

            benchmark::DoNotOptimize(eid);
            benchmark::DoNotOptimize(health);
            benchmark::DoNotOptimize(px);
            (void)tag;
            (void)active;
        }

        totalBytes += json.size();
        benchmark::DoNotOptimize(name);
        benchmark::DoNotOptimize(version);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_RapidJson_Read)->Unit(benchmark::kMillisecond);

// ============================================================
//  ── 8. RapidJSON RoundTrip 벤치 ────────────────────────────
// ============================================================

static void BM_RapidJson_RoundTrip(benchmark::State& state)
{
    const SceneData& scene      = GetScene();
    std::size_t      totalBytes = 0;

    for (auto _: state)
    {
        // Write
        rapidjson::StringBuffer                    sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("name");
        writer.String(scene.name);
        writer.Key("version");
        writer.Int(scene.version);
        writer.Key("entities");
        writer.StartArray();
        for (const auto& e: scene.entities)
        {
            writer.StartObject();
            writer.Key("id");
            writer.Int(e.id);
            writer.Key("tag");
            writer.String(e.tag);
            writer.Key("active");
            writer.Bool(e.active);
            writer.Key("health");
            writer.Double(e.health);
            writer.Key("position");
            writer.StartArray();
            writer.Double(e.pos[0]);
            writer.Double(e.pos[1]);
            writer.Double(e.pos[2]);
            writer.EndArray();
            writer.Key("rotation");
            writer.StartArray();
            writer.Double(e.rot[0]);
            writer.Double(e.rot[1]);
            writer.Double(e.rot[2]);
            writer.Double(e.rot[3]);
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        totalBytes += sb.GetSize();

        // Read
        rapidjson::Document doc;
        doc.Parse(sb.GetString(), sb.GetSize());
        benchmark::DoNotOptimize(doc);
    }

    state.SetBytesProcessed(static_cast<int64_t>(totalBytes));
    state.counters["throughput_MBps"] = benchmark::Counter(
        static_cast<double>(totalBytes),
        benchmark::Counter::kIsRate,
        benchmark::Counter::OneK::kIs1024);
}
BENCHMARK(BM_RapidJson_RoundTrip)->Unit(benchmark::kMillisecond);

/*

    [DEBUG]

    2026-03-21T14:39:39+09:00
    Running C:\Users\Ahnjiwoo\Desktop\JamEngine\JamEngine\x64\Debug\JamBenchmark.exe
    Run on (32 X 2495 MHz CPU s)
    CPU Caches:
      L1 Data 32 KiB (x16)
      L1 Instruction 32 KiB (x16)
      L2 Unified 1024 KiB (x16)
      L3 Unified 32768 KiB (x2)
    ***WARNING*** Library was built as DEBUG. Timings may be affected.
    ---------------------------------------------------------------------------------
    Benchmark                       Time             CPU   Iterations UserCounters...
    ---------------------------------------------------------------------------------
    BM_Jam_Write                0.457 ms        0.425 ms         1545 bytes_per_second=77.4804Mi/s throughput_MBps=77.4804Mi/s
    BM_Jam_Read                 0.631 ms        0.628 ms         1120 bytes_per_second=52.4225Mi/s throughput_MBps=52.4225Mi/s
    BM_Jam_RoundTrip            0.599 ms        0.600 ms         1120 bytes_per_second=54.8608Mi/s throughput_MBps=54.8608Mi/s
    BM_YYJson_Raw_Write         0.303 ms        0.305 ms         2358 bytes_per_second=107.969Mi/s throughput_MBps=107.969Mi/s
    BM_YYJson_Raw_Read          0.250 ms        0.251 ms         2800 bytes_per_second=131.056Mi/s throughput_MBps=131.056Mi/s
    BM_RapidJson_Write          0.776 ms        0.767 ms          896 bytes_per_second=42.8924Mi/s throughput_MBps=42.8924Mi/s
    BM_RapidJson_Read            1.92 ms         1.89 ms          373 bytes_per_second=17.4586Mi/s throughput_MBps=17.4586Mi/s
    BM_RapidJson_RoundTrip       2.33 ms         2.35 ms          299 bytes_per_second=13.9953Mi/s throughput_MBps=13.9953Mi/s
    [2026-03-21 14:39:47.005] [CORE] [trace] app 'JamApspTest' shutdown complete.

    [RELEASE]

    ---------------------------------------------------------------------------------
    Benchmark                       Time             CPU   Iterations UserCounters...
    ---------------------------------------------------------------------------------
    BM_Jam_Write                0.052 ms        0.052 ms        11200 bytes_per_second=637.571Mi/s throughput_MBps=637.571Mi/s
    BM_Jam_Read                 0.046 ms        0.045 ms        14452 bytes_per_second=724.755Mi/s throughput_MBps=724.755Mi/s
    BM_Jam_RoundTrip            0.075 ms        0.075 ms        11200 bytes_per_second=436.854Mi/s throughput_MBps=436.854Mi/s
    BM_YYJson_Raw_Write         0.037 ms        0.038 ms        18667 bytes_per_second=873.724Mi/s throughput_MBps=873.724Mi/s
    BM_YYJson_Raw_Read          0.026 ms        0.025 ms        29867 bytes_per_second=1.27986Gi/s throughput_MBps=1.27986Gi/s
    BM_RapidJson_Write          0.078 ms        0.075 ms         8960 bytes_per_second=438.899Mi/s throughput_MBps=438.899Mi/s
    BM_RapidJson_Read           0.090 ms        0.090 ms         7467 bytes_per_second=365.755Mi/s throughput_MBps=365.755Mi/s
    BM_RapidJson_RoundTrip      0.155 ms        0.157 ms         4978 bytes_per_second=209.706Mi/s throughput_MBps=209.706Mi/s
    [2026-03-21 14:40:31.960] [CORE] [trace] app 'JamAppTest' shutdown complete.

 */