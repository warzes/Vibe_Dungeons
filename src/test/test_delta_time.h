#pragma once

#include "engine/delta_time.h"
#include "test/test_runner.h"
#if !defined(__EMSCRIPTEN__)
#include <thread>
#else
#include <emscripten.h>
#endif

//=============================================================================
// DeltaTime
//=============================================================================

TEST(DeltaTime_ZeroOnFirstTick)
{
	DeltaTime dt;
	dt.Tick();
	EXPECT_NEAR(dt.Seconds(), 0.0f, 1e-6f);
}

TEST(DeltaTime_PositiveAfterWait)
{
	DeltaTime dt;
	dt.Tick(); // first tick = 0
#if !defined(__EMSCRIPTEN__)
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
#else
	emscripten_sleep(1);
#endif
	dt.Tick(); // second tick ~10ms
	EXPECT_TRUE(dt.Seconds() > 0.001f);
	EXPECT_TRUE(dt.Seconds() < 0.1f);
}

TEST(DeltaTime_TotalTimeAccumulates)
{
	DeltaTime dt;
	dt.Tick(); // t=0
#if !defined(__EMSCRIPTEN__)
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
#else
	emscripten_sleep(1);
#endif
	dt.Tick(); // t+=5ms
	EXPECT_TRUE(dt.TotalTime() > 0.001f);
}

TEST(DeltaTime_Milliseconds)
{
	DeltaTime dt;
	dt.Tick();
#if !defined(__EMSCRIPTEN__)
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
#else
	emscripten_sleep(1);
#endif
	dt.Tick();
	float ms = dt.Milliseconds();
	EXPECT_TRUE(ms > 1.0f);
	EXPECT_TRUE(ms < 100.0f);
}
