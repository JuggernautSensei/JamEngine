#include "pch.h"
#include <string>

using namespace Jam;

namespace
{
	std::string MakePayload(const size_t len)
	{
		std::string s;
		s.resize(len);
		for (size_t i = 0; i < len; ++i)
		{
			s[i] = static_cast<char>('a' + (i % 26));
		}
		return s;
	}

	void BuildFindData(const size_t len, std::string& source, std::string& needle)
	{
		source = MakePayload(len);
		needle = "XYZW";
		if (len < needle.size())
		{
			needle = "X";
		}

		const size_t pos = len > needle.size() ? (len / 2) - (needle.size() / 2) : 0;
		for (size_t i = 0; i < needle.size(); ++i)
		{
			source[pos + i] = needle[i];
		}
	}
}   // namespace
//
//static void BM_JamString_Construct(benchmark::State& state)
//{
//	const size_t     len = static_cast<size_t>(state.range(0));
//	const std::string src = MakePayload(len);
//
//	for (auto _: state)
//	{
//		Jam::String s(src.c_str(), static_cast<Jam::SizeT>(src.size()));
//		benchmark::DoNotOptimize(s.GetPtr());
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
//}
//BENCHMARK(BM_JamString_Construct)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_Construct(benchmark::State& state)
//{
//	const size_t     len = static_cast<size_t>(state.range(0));
//	const std::string src = MakePayload(len);
//
//	for (auto _: state)
//	{
//		std::string s(src.c_str(), src.size());
//		benchmark::DoNotOptimize(s.data());
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
//}
//BENCHMARK(BM_StdString_Construct)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_Append(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string baseStd = MakePayload(len);
//	const std::string addStd  = MakePayload(len / 2);
//
//	const Jam::String     base(baseStd.c_str(), static_cast<Jam::SizeT>(baseStd.size()));
//	const Jam::StringView add(addStd.c_str(), static_cast<Jam::SizeT>(addStd.size()));
//
//	for (auto _: state)
//	{
//		Jam::String s(base);
//		s.Append(add);
//		benchmark::DoNotOptimize(s.GetLength());
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * (len + addStd.size())));
//}
//BENCHMARK(BM_JamString_Append)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_Append(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string base = MakePayload(len);
//	const std::string add  = MakePayload(len / 2);
//
//	for (auto _: state)
//	{
//		std::string s = base;
//		s.append(add);
//		benchmark::DoNotOptimize(s.size());
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * (len + add.size())));
//}
//BENCHMARK(BM_StdString_Append)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_JamString_Find(benchmark::State& state)
{
	const size_t len = static_cast<size_t>(state.range(0));
	std::string  source;
	std::string  needle;
	BuildFindData(len, source, needle);

	const Jam::StringView sourceView(source.c_str(), static_cast<Jam::SizeT>(source.size()));
	const Jam::StringView needleView(needle.c_str(), static_cast<Jam::SizeT>(needle.size()));

	for (auto _: state)
	{
		const CharResult result = sourceView.Find(needleView);
		benchmark::DoNotOptimize(result.pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_JamString_Find)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_StdString_Find(benchmark::State& state)
{
	const size_t len = static_cast<size_t>(state.range(0));
	std::string  source;
	std::string  needle;
	BuildFindData(len, source, needle);

	for (auto _: state)
	{
		const auto pos = source.find(needle);
		benchmark::DoNotOptimize(pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_StdString_Find)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_JamString_FindChar(benchmark::State& state)
{
	const size_t     len = static_cast<size_t>(state.range(0));
	const std::string src = MakePayload(len);
	const Jam::StringView view(src.c_str(), static_cast<Jam::SizeT>(src.size()));
	const char target = 'm';

	for (auto _: state)
	{
		const CharResult result = view.Find(target);
		benchmark::DoNotOptimize(result.pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_JamString_FindChar)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_StdString_FindChar(benchmark::State& state)
{
	const size_t     len = static_cast<size_t>(state.range(0));
	const std::string src = MakePayload(len);
	const char target = 'm';

	for (auto _: state)
	{
		const auto pos = src.find(target);
		benchmark::DoNotOptimize(pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_StdString_FindChar)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_JamString_RFind(benchmark::State& state)
{
	const size_t len = static_cast<size_t>(state.range(0));
	std::string  source;
	std::string  needle;
	BuildFindData(len, source, needle);

	const Jam::StringView sourceView(source.c_str(), static_cast<Jam::SizeT>(source.size()));
	const Jam::StringView needleView(needle.c_str(), static_cast<Jam::SizeT>(needle.size()));

	for (auto _: state)
	{
		const CharResult result = sourceView.RFind(needleView);
		benchmark::DoNotOptimize(result.pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_JamString_RFind)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_StdString_RFind(benchmark::State& state)
{
	const size_t len = static_cast<size_t>(state.range(0));
	std::string  source;
	std::string  needle;
	BuildFindData(len, source, needle);

	for (auto _: state)
	{
		const auto pos = source.rfind(needle);
		benchmark::DoNotOptimize(pos);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
}
BENCHMARK(BM_StdString_RFind)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_StartsWith(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string source = MakePayload(len);
//	const std::string prefix = source.substr(0, 8);
//
//	const Jam::StringView sourceView(source.c_str(), static_cast<Jam::SizeT>(source.size()));
//	const Jam::StringView prefixView(prefix.c_str(), static_cast<Jam::SizeT>(prefix.size()));
//
//	for (auto _: state)
//	{
//		const bool ok = sourceView.StartsWith(prefixView);
//		benchmark::DoNotOptimize(ok);
//	}
//}
//BENCHMARK(BM_JamString_StartsWith)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_StartsWith(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string source = MakePayload(len);
//	const std::string prefix = source.substr(0, 8);
//
//	for (auto _: state)
//	{
//		const bool ok = source.compare(0, prefix.size(), prefix) == 0;
//		benchmark::DoNotOptimize(ok);
//	}
//}
//BENCHMARK(BM_StdString_StartsWith)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_EndsWith(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string source = MakePayload(len);
//	const std::string suffix = source.substr(source.size() > 8 ? source.size() - 8 : 0);
//
//	const Jam::StringView sourceView(source.c_str(), static_cast<Jam::SizeT>(source.size()));
//	const Jam::StringView suffixView(suffix.c_str(), static_cast<Jam::SizeT>(suffix.size()));
//
//	for (auto _: state)
//	{
//		const bool ok = sourceView.EndsWith(suffixView);
//		benchmark::DoNotOptimize(ok);
//	}
//}
//BENCHMARK(BM_JamString_EndsWith)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_EndsWith(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string source = MakePayload(len);
//	const std::string suffix = source.substr(source.size() > 8 ? source.size() - 8 : 0);
//
//	for (auto _: state)
//	{
//		const size_t start = source.size() - suffix.size();
//		const bool   ok    = source.compare(start, suffix.size(), suffix) == 0;
//		benchmark::DoNotOptimize(ok);
//	}
//}
//BENCHMARK(BM_StdString_EndsWith)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_CompareEqual(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string a = MakePayload(len);
//	const std::string b = a;
//
//	const Jam::StringView aView(a.c_str(), static_cast<Jam::SizeT>(a.size()));
//	const Jam::StringView bView(b.c_str(), static_cast<Jam::SizeT>(b.size()));
//
//	for (auto _: state)
//	{
//		const bool eq = aView.CompareEqual(bView);
//		benchmark::DoNotOptimize(eq);
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
//}
//BENCHMARK(BM_JamString_CompareEqual)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_CompareEqual(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string a = MakePayload(len);
//	const std::string b = a;
//
//	for (auto _: state)
//	{
//		const bool eq = (a == b);
//		benchmark::DoNotOptimize(eq);
//	}
//
//	state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * len));
//}
//BENCHMARK(BM_StdString_CompareEqual)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_Compare(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string a = MakePayload(len);
//	std::string       b = a;
//	b[b.size() / 2] = static_cast<char>(b[b.size() / 2] + 1);
//
//	const Jam::StringView aView(a.c_str(), static_cast<Jam::SizeT>(a.size()));
//	const Jam::StringView bView(b.c_str(), static_cast<Jam::SizeT>(b.size()));
//
//	for (auto _: state)
//	{
//		const Jam::SizeT cmp = aView.Compare(bView);
//		benchmark::DoNotOptimize(cmp);
//	}
//}
//BENCHMARK(BM_JamString_Compare)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_Compare(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string a = MakePayload(len);
//	std::string       b = a;
//	b[b.size() / 2] = static_cast<char>(b[b.size() / 2] + 1);
//
//	for (auto _: state)
//	{
//		const int cmp = a.compare(b);
//		benchmark::DoNotOptimize(cmp);
//	}
//}
//BENCHMARK(BM_StdString_Compare)->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_JamString_InsertErase(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string base = MakePayload(len);
//	const std::string insertStd = MakePayload(8);
//
//	const Jam::String     insertStr(insertStd.c_str(), static_cast<Jam::SizeT>(insertStd.size()));
//	const Jam::StringView insertView(insertStr.GetPtr(), insertStr.GetLength());
//
//	for (auto _: state)
//	{
//		Jam::String s(base.c_str(), static_cast<Jam::SizeT>(base.size()));
//		const Jam::SizeT pos = static_cast<Jam::SizeT>(s.GetLength() / 2);
//		s.Insert(pos, insertView);
//		s.Erase(pos, insertView.GetLength());
//		benchmark::DoNotOptimize(s.GetLength());
//	}
//}
//BENCHMARK(BM_JamString_InsertErase)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
//
//static void BM_StdString_InsertErase(benchmark::State& state)
//{
//	const size_t      len = static_cast<size_t>(state.range(0));
//	const std::string base = MakePayload(len);
//	const std::string insertStr = MakePayload(8);
//
//	for (auto _: state)
//	{
//		std::string s = base;
//		const size_t pos = s.size() / 2;
//		s.insert(pos, insertStr);
//		s.erase(pos, insertStr.size());
//		benchmark::DoNotOptimize(s.size());
//	}
//}
//BENCHMARK(BM_StdString_InsertErase)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
