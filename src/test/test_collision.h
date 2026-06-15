#pragma once

#include "core/collision.h"
#include "test/test_runner.h"

//=============================================================================
// Collision
//=============================================================================

TEST(Collision_AABBvsAABB_Overlap)
{
	AABB a, b;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(2, 2, 2);
	b.min = glm::vec3(1, 1, 1); b.max = glm::vec3(3, 3, 3);
	EXPECT_TRUE(Collision::AABBvsAABB(a, b));
}

TEST(Collision_AABBvsAABB_NoOverlap)
{
	AABB a, b;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(1, 1, 1);
	b.min = glm::vec3(5, 5, 5); b.max = glm::vec3(6, 6, 6);
	EXPECT_FALSE(Collision::AABBvsAABB(a, b));
}

TEST(Collision_AABBvsAABB_Touching)
{
	AABB a, b;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(2, 2, 2);
	b.min = glm::vec3(2, 2, 2); b.max = glm::vec3(4, 4, 4);
	EXPECT_TRUE(Collision::AABBvsAABB(a, b));
}

TEST(Collision_AABBvsAABB_Contains)
{
	AABB a, b;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(10, 10, 10);
	b.min = glm::vec3(2, 2, 2); b.max = glm::vec3(5, 5, 5);
	EXPECT_TRUE(Collision::AABBvsAABB(a, b));
}

TEST(Collision_AABBvsAABB_OneAxisSeparated)
{
	AABB a, b;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(2, 2, 2);
	b.min = glm::vec3(0, 5, 0); b.max = glm::vec3(2, 7, 2);
	EXPECT_FALSE(Collision::AABBvsAABB(a, b));
}

TEST(Collision_SphereVsSphere_Overlap)
{
	Sphere a, b;
	a.center = glm::vec3(0, 0, 0); a.radius = 2.0f;
	b.center = glm::vec3(1, 1, 0); b.radius = 1.0f;
	EXPECT_TRUE(Collision::SphereVsSphere(a, b));
}

TEST(Collision_SphereVsSphere_NoOverlap)
{
	Sphere a, b;
	a.center = glm::vec3(0, 0, 0); a.radius = 1.0f;
	b.center = glm::vec3(10, 0, 0); b.radius = 1.0f;
	EXPECT_FALSE(Collision::SphereVsSphere(a, b));
}

TEST(Collision_SphereVsSphere_Touching)
{
	Sphere a, b;
	a.center = glm::vec3(0, 0, 0); a.radius = 2.0f;
	b.center = glm::vec3(4, 0, 0); b.radius = 2.0f;
	EXPECT_TRUE(Collision::SphereVsSphere(a, b));
}

TEST(Collision_SphereVsAABB_Inside)
{
	Sphere s;
	s.center = glm::vec3(2, 2, 2); s.radius = 1.0f;
	AABB a;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(5, 5, 5);
	EXPECT_TRUE(Collision::SphereVsAABB(s, a));
}

TEST(Collision_SphereVsAABB_Outside)
{
	Sphere s;
	s.center = glm::vec3(10, 10, 10); s.radius = 1.0f;
	AABB a;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(5, 5, 5);
	EXPECT_FALSE(Collision::SphereVsAABB(s, a));
}

TEST(Collision_SphereVsAABB_CornerTouch)
{
	Sphere s;
	s.center = glm::vec3(-1, -1, -1); s.radius = 1.8f; // distance to corner (0,0,0) = sqrt(3) ≈ 1.732
	AABB a;
	a.min = glm::vec3(0, 0, 0); a.max = glm::vec3(3, 3, 3);
	EXPECT_TRUE(Collision::SphereVsAABB(s, a));
}

TEST(Collision_RayVsAABB_Hit)
{
	AABB a;
	a.min = glm::vec3(-1, -1, -1); a.max = glm::vec3(1, 1, 1);
	float tMin, tMax;
	EXPECT_TRUE(Collision::RayVsAABB(glm::vec3(-5, 0, 0), glm::vec3(1, 0, 0), a, tMin, tMax));
	EXPECT_NEAR(tMin, 4.0f, 1e-6f);
	EXPECT_NEAR(tMax, 6.0f, 1e-6f);
}

TEST(Collision_RayVsAABB_Miss)
{
	AABB a;
	a.min = glm::vec3(-1, -1, -1); a.max = glm::vec3(1, 1, 1);
	float tMin, tMax;
	EXPECT_FALSE(Collision::RayVsAABB(glm::vec3(-5, 10, 0), glm::vec3(1, 0, 0), a, tMin, tMax));
}

TEST(Collision_RayVsAABB_OriginInside)
{
	AABB a;
	a.min = glm::vec3(-1, -1, -1); a.max = glm::vec3(1, 1, 1);
	float tMin, tMax;
	EXPECT_TRUE(Collision::RayVsAABB(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), a, tMin, tMax));
	EXPECT_NEAR(tMin, -1.0f, 1e-6f);
}

TEST(Collision_RayVsAABB_Parallel)
{
	AABB a;
	a.min = glm::vec3(2, -1, -1); a.max = glm::vec3(3, 1, 1);
	float tMin, tMax;
	EXPECT_FALSE(Collision::RayVsAABB(glm::vec3(0, -5, 0), glm::vec3(1, 0, 0), a, tMin, tMax));
}

TEST(Collision_RayVsSphere_Hit)
{
	Sphere s;
	s.center = glm::vec3(0, 0, 0); s.radius = 1.0f;
	float t;
	EXPECT_TRUE(Collision::RayVsSphere(glm::vec3(-5, 0, 0), glm::vec3(1, 0, 0), s, t));
	EXPECT_NEAR(t, 4.0f, 1e-6f);
}

TEST(Collision_RayVsSphere_Miss)
{
	Sphere s;
	s.center = glm::vec3(0, 0, 0); s.radius = 1.0f;
	float t;
	EXPECT_FALSE(Collision::RayVsSphere(glm::vec3(-5, 10, 0), glm::vec3(1, 0, 0), s, t));
}

TEST(Collision_RayVsSphere_OriginInside)
{
	Sphere s;
	s.center = glm::vec3(0, 0, 0); s.radius = 5.0f;
	float t;
	EXPECT_TRUE(Collision::RayVsSphere(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), s, t));
	EXPECT_NEAR(t, -5.0f, 1e-6f);
}
