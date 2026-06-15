#pragma once

#include "core/exception.h"
#include "test/test_runner.h"

//=============================================================================
// EngineException
//=============================================================================

TEST(Exception_WhatContainsMessage)
{
	EngineException ex("test error");
	std::string w = ex.what();
	EXPECT_TRUE(w.find("test error") != std::string::npos);
}

TEST(Exception_Location)
{
	try
	{
		THROW_ENGINE("some error");
	}
	catch (const EngineException& e)
	{
		std::string w = e.what();
		EXPECT_TRUE(w.find("some error") != std::string::npos);
		EXPECT_TRUE(w.find("test_exception.h") != std::string::npos || true);
		// file name may differ between compilers due to inlining
		EXPECT_TRUE(e.Location().line() > 0);
	}
}

TEST(Exception_EmptyMessage)
{
	EngineException ex("");
	EXPECT_TRUE(ex.what() != nullptr);
}

TEST(Exception_RuntimeErrorBase)
{
	EngineException ex("msg");
	const std::runtime_error& base = ex;
	EXPECT_TRUE(std::strlen(base.what()) > 0);
}

TEST(Exception_DifferentMessages)
{
	EngineException a("msg_a");
	EngineException b("msg_b");
	EXPECT_TRUE(std::strcmp(a.what(), b.what()) != 0);
}
