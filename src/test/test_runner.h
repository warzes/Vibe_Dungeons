#pragma once

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <string_view>
#include <functional>
#include <vector>

//=============================================================================
// Minimal test framework: no external dependencies
//=============================================================================

struct TestFailure final
{
	std::string file;
	int line;
	std::string expr;
};

#define EXPECT_TRUE(cond) do { \
	if (!(cond)) { TestRunner::Fail({__FILE__, __LINE__, #cond}); return; } \
} while(0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(a, b) do { \
	auto _a = (a); auto _b = (b); \
	if (!(_a == _b)) { \
		TestRunner::Fail({__FILE__, __LINE__, #a " == " #b}); return; \
	} \
} while(0)

#define EXPECT_NEAR(a, b, eps) do { \
	auto _a = (a); auto _b = (b); \
	if (std::abs(static_cast<double>(_a - _b)) > static_cast<double>(eps)) { \
		TestRunner::Fail({__FILE__, __LINE__, #a " ~= " #b}); return; \
	} \
} while(0)

#define EXPECT_STREQ(a, b) do { \
	auto _a = (a); auto _b = (b); \
	if (_a != _b) { \
		TestRunner::Fail({__FILE__, __LINE__, #a " == " #b}); return; \
	} \
} while(0)

#define TEST(name) static void test_##name(); \
struct TestReg_##name final { \
	TestReg_##name() { TestRunner::Register(#name, test_##name); } \
} g_reg_##name; \
static void test_##name()

class TestRunner final
{
public:
	using TestFunc = std::function<void()>;

	static void Register(std::string_view name, TestFunc func)
	{
		Instance().m_tests.push_back({std::string(name), std::move(func)});
	}

	static void Fail(TestFailure failure)
	{
		Instance().m_failures.push_back(std::move(failure));
	}

	static int RunAll(const char* logPath)
	{
		auto& inst = Instance();

		FILE* log;
		fopen_s(&log, logPath, "w");
		if (!log)
		{
			std::fprintf(stderr, "[TEST] Cannot open log: %s\n", logPath);
			return 1;
		}

		std::fprintf(log, "=== Dungeon Crawlers Test Suite ===\n");
		std::fprintf(log, "Tests registered: %zu\n\n", inst.m_tests.size());
		std::printf("[TEST] Running %zu tests...\n", inst.m_tests.size());

		int passed = 0;
		int failed = 0;

		for (const auto& t : inst.m_tests)
		{
			inst.m_failures.clear();

			t.func();

			if (inst.m_failures.empty())
			{
				std::fprintf(log, "  PASS  %s\n", t.name.c_str());
				std::printf("[TEST] PASS  %s\n", t.name.c_str());
				++passed;
			}
			else
			{
				std::fprintf(log, "  FAIL  %s\n", t.name.c_str());
				std::printf("[TEST] FAIL  %s\n", t.name.c_str());
				for (const auto& f : inst.m_failures)
				{
					std::fprintf(log, "    %s:%d: %s\n", f.file.c_str(), f.line, f.expr.c_str());
					std::printf("[TEST]   %s:%d: %s\n", f.file.c_str(), f.line, f.expr.c_str());
				}
				++failed;
			}
		}

		std::fprintf(log, "\n=== Results: %d passed, %d failed, %zu total ===\n",
			passed, failed, inst.m_tests.size());
		std::printf("[TEST] Results: %d passed, %d failed, %zu total\n",
			passed, failed, inst.m_tests.size());

		std::fclose(log);

		return failed;
	}

private:
	struct TestEntry
	{
		std::string name;
		TestFunc func;
	};

	std::vector<TestEntry> m_tests;
	std::vector<TestFailure> m_failures;

	static TestRunner& Instance()
	{
		static TestRunner inst;
		return inst;
	}
};
