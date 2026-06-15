#pragma once

#include "engine/renderer/frustum.h"
#include "test/test_runner.h"

//=============================================================================
// Frustum (pure math, no GL)
//=============================================================================

TEST(Frustum_BoxInside)
{
	Frustum f;
	// Identity VP: near plane at -1, far at 1 in NDC
	// Simple ortho-like frustum: visible box at origin
	glm::mat4 vp = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
	f.Extract(vp);

	AABB box;
	box.min = glm::vec3(-1, -1, -1);
	box.max = glm::vec3(1, 1, 1);
	EXPECT_TRUE(f.TestAABB(box));
}

TEST(Frustum_BoxOutside)
{
	Frustum f;
	glm::mat4 vp = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
	f.Extract(vp);

	AABB box;
	box.min = glm::vec3(100, 100, 100);
	box.max = glm::vec3(200, 200, 200);
	EXPECT_FALSE(f.TestAABB(box));
}

TEST(Frustum_BoxPartlyInside)
{
	Frustum f;
	glm::mat4 vp = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
	f.Extract(vp);

	AABB box;
	box.min = glm::vec3(3, -1, -1);
	box.max = glm::vec3(7, 1, 1);
	EXPECT_TRUE(f.TestAABB(box)); // Partly inside (x 3..7 overlaps -5..5)
}

TEST(Frustum_PerspectiveFrustum)
{
	Frustum f;
	glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	f.Extract(proj * view);

	AABB box;
	box.min = glm::vec3(-1, -1, -5);
	box.max = glm::vec3(1, 1, -3);
	EXPECT_TRUE(f.TestAABB(box));
}

TEST(Frustum_BoxBehindCamera)
{
	Frustum f;
	glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	f.Extract(proj * view);

	AABB box;
	box.min = glm::vec3(-1, -1, 6);
	box.max = glm::vec3(1, 1, 10);
	EXPECT_FALSE(f.TestAABB(box));
}

TEST(Frustum_BoxAtFarPlane)
{
	Frustum f;
	glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 10.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	f.Extract(proj * view);

	AABB box;
	box.min = glm::vec3(-1, -1, -15);
	box.max = glm::vec3(1, 1, -13);
	EXPECT_FALSE(f.TestAABB(box)); // Beyond far plane (z=-15 < -10)
}
