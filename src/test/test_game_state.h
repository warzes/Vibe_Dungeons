#pragma once

#include "engine/game_state.h"
#include "engine/delta_time.h"
#include "test/test_runner.h"

//=============================================================================
// GameStateMachine
//=============================================================================

class TestState final : public GameState
{
public:
	std::string id;
	int enterCount = 0;
	int exitCount = 0;
	int pauseCount = 0;
	int resumeCount = 0;
	int updateCount = 0;

	explicit TestState(std::string_view id_) : id(id_) {}

	void OnEnter() noexcept override { ++enterCount; }
	void OnExit() noexcept override { ++exitCount; }
	void OnPause() noexcept override { ++pauseCount; }
	void OnResume() noexcept override { ++resumeCount; }

	void HandleEvent(const SDL_Event&) noexcept override {}
	void Update(const DeltaTime&) noexcept override { ++updateCount; }
	void Render() noexcept override {}

	[[nodiscard]] std::string_view GetName() const noexcept override { return id; }
};

TEST(GameStateMachine_Empty)
{
	GameStateMachine sm;
	EXPECT_TRUE(sm.IsEmpty());
	EXPECT_EQ(sm.CurrentState(), nullptr);
}

TEST(GameStateMachine_PushState)
{
	GameStateMachine sm;
	sm.PushState(std::make_unique<TestState>("A"));
	sm.ProcessActions();
	EXPECT_FALSE(sm.IsEmpty());
	EXPECT_STREQ(sm.CurrentState()->GetName(), "A");
}

TEST(GameStateMachine_PopState)
{
	GameStateMachine sm;
	sm.PushState(std::make_unique<TestState>("A"));
	sm.ProcessActions();
	sm.PopState();
	sm.ProcessActions();
	EXPECT_TRUE(sm.IsEmpty());
}

TEST(GameStateMachine_ReplaceState)
{
	GameStateMachine sm;
	sm.PushState(std::make_unique<TestState>("A"));
	sm.ProcessActions();
	sm.ReplaceState(std::make_unique<TestState>("B"));
	sm.ProcessActions();
	EXPECT_STREQ(sm.CurrentState()->GetName(), "B");
	EXPECT_EQ(sm.CurrentState()->GetName(), sm.CurrentState()->GetName());
}

TEST(GameStateMachine_Lifecycle)
{
	GameStateMachine sm;
	auto s1 = std::make_unique<TestState>("A");
	auto* p1 = s1.get();
	sm.PushState(std::move(s1));
	sm.ProcessActions();

	EXPECT_EQ(p1->enterCount, 1);

	auto s2 = std::make_unique<TestState>("B");
	auto* p2 = s2.get();
	sm.PushState(std::move(s2));
	sm.ProcessActions();

	EXPECT_EQ(p1->pauseCount, 1);
	EXPECT_EQ(p2->enterCount, 1);

	sm.PopState();
	sm.ProcessActions();

	// p2 destroyed by ProcessActions — verify pop lifecycle indirectly
	EXPECT_EQ(p1->resumeCount, 1);  // A resumed when B popped
	EXPECT_EQ(sm.CurrentState(), p1);
}

TEST(GameStateMachine_RegisterAndPushByName)
{
	GameStateMachine sm;
	sm.RegisterState("Test", []() { return std::make_unique<TestState>("A"); });
	sm.PushState("Test");
	sm.ProcessActions();
	EXPECT_FALSE(sm.IsEmpty());
	EXPECT_STREQ(sm.CurrentState()->GetName(), "A");
}

TEST(GameStateMachine_ReplaceByName)
{
	GameStateMachine sm;
	sm.RegisterState("A", []() { return std::make_unique<TestState>("A"); });
	sm.RegisterState("B", []() { return std::make_unique<TestState>("B"); });
	sm.PushState("A");
	sm.ProcessActions();
	sm.ReplaceState("B");
	sm.ProcessActions();
	EXPECT_STREQ(sm.CurrentState()->GetName(), "B");
}

TEST(GameStateMachine_UpdateDelegation)
{
	GameStateMachine sm;
	auto st = std::make_unique<TestState>("A");
	auto* pst = st.get();
	sm.PushState(std::move(st));
	sm.ProcessActions();

	DeltaTime dt;
	dt.Tick();
	sm.Update(dt);
	EXPECT_EQ(pst->updateCount, 1);
}

TEST(GameStateMachine_MultipleActions)
{
	GameStateMachine sm;
	auto s1 = std::make_unique<TestState>("A");
	auto s2 = std::make_unique<TestState>("B");
	auto s3 = std::make_unique<TestState>("C");

	sm.PushState(std::move(s1));
	sm.PushState(std::move(s2));
	sm.PopState();
	sm.PushState(std::move(s3));
	sm.ProcessActions();

	EXPECT_STREQ(sm.CurrentState()->GetName(), "C");
}
