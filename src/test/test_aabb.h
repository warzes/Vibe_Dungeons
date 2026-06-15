#pragma once

#include "core/aabb.h"
#include "test/test_runner.h"

//=============================================================================
// AABB
//=============================================================================

TEST(AABB_DefaultMinMax)
{
	AABB a;
	EXPECT_EQ(a.min.x, std::numeric_limits<float>::max());
	EXPECT_EQ(a.max.x, -std::numeric_limits<float>::max());
}

TEST(AABB_IdentityTransform)
{
	AABB a;
	a.min = glm::vec3(-1, -2, -3);
	a.max = glm::vec3(4, 5, 6);
	AABB result = a.Transform(glm::mat4(1.0f));
	EXPECT_NEAR(result.min.x, -1.0f, 1e-6f);
	EXPECT_NEAR(result.min.y, -2.0f, 1e-6f);
	EXPECT_NEAR(result.min.z, -3.0f, 1e-6f);
	EXPECT_NEAR(result.max.x, 4.0f, 1e-6f);
	EXPECT_NEAR(result.max.y, 5.0f, 1e-6f);
	EXPECT_NEAR(result.max.z, 6.0f, 1e-6f);
}

TEST(AABB_TranslateTransform)
{
	AABB a;
	a.min = glm::vec3(-1, -1, -1);
	a.max = glm::vec3(1, 1, 1);
	glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(10, 20, 30));
	AABB result = a.Transform(t);
	EXPECT_NEAR(result.min.x, 9.0f, 1e-6f);
	EXPECT_NEAR(result.min.y, 19.0f, 1e-6f);
	EXPECT_NEAR(result.min.z, 29.0f, 1e-6f);
	EXPECT_NEAR(result.max.x, 11.0f, 1e-6f);
	EXPECT_NEAR(result.max.y, 21.0f, 1e-6f);
	EXPECT_NEAR(result.max.z, 31.0f, 1e-6f);
}

TEST(AABB_ScaleTransform)
{
	AABB a;
	a.min = glm::vec3(-1, -2, -3);
	a.max = glm::vec3(2, 4, 6);
	glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(2, 0.5f, 3));
	AABB result = a.Transform(s);
	EXPECT_NEAR(result.min.x, -2.0f, 1e-6f);
	EXPECT_NEAR(result.min.y, -1.0f, 1e-6f);
	EXPECT_NEAR(result.min.z, -9.0f, 1e-6f);
	EXPECT_NEAR(result.max.x, 4.0f, 1e-6f);
	EXPECT_NEAR(result.max.y, 2.0f, 1e-6f);
	EXPECT_NEAR(result.max.z, 18.0f, 1e-6f);
}

TEST(AABB_ZeroSize)
{
	AABB a;
	a.min = glm::vec3(5, 5, 5);
	a.max = glm::vec3(5, 5, 5);
	AABB result = a.Transform(glm::mat4(1.0f));
	EXPECT_NEAR(result.min.x, 5.0f, 1e-6f);
	EXPECT_NEAR(result.max.x, 5.0f, 1e-6f);
}
