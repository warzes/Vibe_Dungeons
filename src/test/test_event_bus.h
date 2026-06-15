#pragma once

#include "core/event_bus.h"
#include "test/test_runner.h"

//=============================================================================
// EventBus
//=============================================================================

struct TestEventA final : EventBase
{
	int value = 0;
	explicit TestEventA(int v = 0) : value(v) {}
};

struct TestEventB final : EventBase
{
	std::string text;
	explicit TestEventB(std::string_view t = "") : text(t) {}
};

TEST(EventBus_SubscribeAndPublish)
{
	EventBus bus;
	int received = 0;
	bus.Subscribe<TestEventA>([&](const TestEventA& e) { received = e.value; });
	bus.Publish(TestEventA{42});
	EXPECT_EQ(received, 42);
}

TEST(EventBus_MultipleSubscribers)
{
	EventBus bus;
	int a = 0, b = 0;
	bus.Subscribe<TestEventA>([&](const TestEventA& e) { a = e.value; });
	bus.Subscribe<TestEventA>([&](const TestEventA& e) { b = e.value * 2; });
	bus.Publish(TestEventA{10});
	EXPECT_EQ(a, 10);
	EXPECT_EQ(b, 20);
}

TEST(EventBus_Unsubscribe)
{
	EventBus bus;
	int count = 0;
	auto token = bus.Subscribe<TestEventA>([&](const TestEventA&) { ++count; });
	bus.Publish(TestEventA{0});
	EXPECT_EQ(count, 1);
	bus.Unsubscribe<TestEventA>(token);
	bus.Publish(TestEventA{0});
	EXPECT_EQ(count, 1);
}

TEST(EventBus_UnsubscribeOtherType)
{
	EventBus bus;
	int count = 0;
	auto token = bus.Subscribe<TestEventA>([&](const TestEventA&) { ++count; });
	bus.Unsubscribe<TestEventB>(token); // wrong type — should not affect
	bus.Publish(TestEventA{0});
	EXPECT_EQ(count, 1);
}

TEST(EventBus_Clear)
{
	EventBus bus;
	int count = 0;
	bus.Subscribe<TestEventA>([&](const TestEventA&) { ++count; });
	bus.Clear();
	bus.Publish(TestEventA{0});
	EXPECT_EQ(count, 0);
}

TEST(EventBus_NoSubscribers)
{
	EventBus bus;
	// Should not crash
	bus.Publish(TestEventA{99});
	bus.Publish(TestEventB{"hello"});
}

TEST(EventBus_DifferentTypes)
{
	EventBus bus;
	int intVal = 0;
	std::string strVal;
	bus.Subscribe<TestEventA>([&](const TestEventA& e) { intVal = e.value; });
	bus.Subscribe<TestEventB>([&](const TestEventB& e) { strVal = e.text; });
	bus.Publish(TestEventA{7});
	bus.Publish(TestEventB{"world"});
	EXPECT_EQ(intVal, 7);
	EXPECT_STREQ(strVal, "world");
}

TEST(EventBus_TokenUniqueness)
{
	EventBus bus;
	auto t1 = bus.Subscribe<TestEventA>([](const TestEventA&) {});
	auto t2 = bus.Subscribe<TestEventA>([](const TestEventA&) {});
	EXPECT_TRUE(t1 != t2);
}

TEST(EventBus_UnsubscribeRemovesOnlyToken)
{
	EventBus bus;
	int a = 0, b = 0;
	auto t1 = bus.Subscribe<TestEventA>([&](const TestEventA&) { a = 1; });
	auto t2 = bus.Subscribe<TestEventA>([&](const TestEventA&) { b = 2; });
	(void)t2;
	bus.Unsubscribe<TestEventA>(t1);
	bus.Publish(TestEventA{0});
	EXPECT_EQ(a, 0);
	EXPECT_EQ(b, 2);
}
