#pragma once

#include "core/transform.h"
#include "test/test_runner.h"

//=============================================================================
// Transform
//=============================================================================

TEST(Transform_DefaultIsIdentity)
{
	Transform t;
	const glm::mat4& m = t.LocalMatrix();
	EXPECT_NEAR(m[0][0], 1.0f, 1e-6f);
	EXPECT_NEAR(m[1][1], 1.0f, 1e-6f);
	EXPECT_NEAR(m[2][2], 1.0f, 1e-6f);
	EXPECT_NEAR(m[3][3], 1.0f, 1e-6f);
}

TEST(Transform_SetPosition)
{
	Transform t;
	t.SetPosition(glm::vec3(10, 20, 30));
	glm::mat4 m = t.LocalMatrix();
	EXPECT_NEAR(m[3][0], 10.0f, 1e-6f);
	EXPECT_NEAR(m[3][1], 20.0f, 1e-6f);
	EXPECT_NEAR(m[3][2], 30.0f, 1e-6f);
}

TEST(Transform_SetScale)
{
	Transform t;
	t.SetScale(glm::vec3(2, 3, 4));
	glm::mat4 m = t.LocalMatrix();
	EXPECT_NEAR(m[0][0], 2.0f, 1e-6f);
	EXPECT_NEAR(m[1][1], 3.0f, 1e-6f);
	EXPECT_NEAR(m[2][2], 4.0f, 1e-6f);
}

TEST(Transform_Compose)
{
	Transform t;
	t.SetPosition(glm::vec3(1, 2, 3));
	t.SetScale(glm::vec3(2, 2, 2));
	glm::mat4 m = t.LocalMatrix();
	// pos*scale: position still at (1,2,3)
	EXPECT_NEAR(m[3][0], 1.0f, 1e-6f);
	EXPECT_NEAR(m[3][1], 2.0f, 1e-6f);
	EXPECT_NEAR(m[3][2], 3.0f, 1e-6f);
	EXPECT_NEAR(m[0][0], 2.0f, 1e-6f);
}

TEST(Transform_DirtyCache)
{
	Transform t;
	(void)t.LocalMatrix(); // cache built
	t.SetPosition(glm::vec3(5, 0, 0));
	glm::mat4 m2 = t.LocalMatrix(); // rebuild
	EXPECT_NEAR(m2[3][0], 5.0f, 1e-6f);
}

TEST(Transform_ParentChild)
{
	Transform parent, child;
	child.SetPosition(glm::vec3(2, 0, 0));
	child.SetParent(&parent);
	parent.SetPosition(glm::vec3(10, 0, 0));

	glm::mat4 childWorld = child.WorldMatrix();
	EXPECT_NEAR(childWorld[3][0], 12.0f, 1e-6f);
}

TEST(Transform_ChildrenList)
{
	Transform parent, c1, c2;
	c1.SetParent(&parent);
	c2.SetParent(&parent);
	EXPECT_EQ(parent.GetChildren().size(), size_t(2));
}

TEST(Transform_Reparent)
{
	Transform oldP, newP, child;
	child.SetParent(&oldP);
	EXPECT_EQ(oldP.GetChildren().size(), size_t(1));
	child.SetParent(&newP);
	EXPECT_EQ(oldP.GetChildren().size(), size_t(0));
	EXPECT_EQ(newP.GetChildren().size(), size_t(1));
}

TEST(Transform_RemoveParent)
{
	Transform parent, child;
	child.SetParent(&parent);
	child.SetParent(nullptr);
	EXPECT_EQ(parent.GetChildren().size(), size_t(0));
	EXPECT_EQ(child.GetParent(), nullptr);
}

TEST(Transform_NestedHierarchy)
{
	Transform grandparent, parent, child;
	grandparent.SetPosition(glm::vec3(1, 0, 0));
	parent.SetPosition(glm::vec3(2, 0, 0));
	child.SetPosition(glm::vec3(3, 0, 0));
	parent.SetParent(&grandparent);
	child.SetParent(&parent);

	glm::mat4 childWorld = child.WorldMatrix();
	EXPECT_NEAR(childWorld[3][0], 6.0f, 1e-6f);
}
