#pragma once

#include "engine/renderer/camera.h"
#include "test/test_runner.h"

//=============================================================================
// Camera (pure math, no GL)
//=============================================================================

TEST(Camera_DefaultPosition)
{
	Camera cam;
	EXPECT_NEAR(cam.Position().z, 3.0f, 1e-6f);
	EXPECT_NEAR(cam.Position().x, 0.0f, 1e-6f);
}

TEST(Camera_SetPosition)
{
	Camera cam;
	cam.SetPosition(glm::vec3(10, 20, 30));
	EXPECT_NEAR(cam.Position().x, 10.0f, 1e-6f);
	EXPECT_NEAR(cam.Position().y, 20.0f, 1e-6f);
	EXPECT_NEAR(cam.Position().z, 30.0f, 1e-6f);
}

TEST(Camera_DefaultForward)
{
	Camera cam;
	// Default yaw=-90 → forward = (0, 0, -1)
	EXPECT_NEAR(cam.Forward().x, 0.0f, 1e-6f);
	EXPECT_NEAR(cam.Forward().y, 0.0f, 1e-6f);
	EXPECT_NEAR(cam.Forward().z, -1.0f, 1e-4f);
}

TEST(Camera_SetRotation)
{
	Camera cam;
	cam.SetRotation(0.0f, 0.0f); // yaw=0 → forward = cos(0) = +X
	EXPECT_NEAR(cam.Forward().x, 1.0f, 1e-4f);
	EXPECT_NEAR(cam.Forward().y, 0.0f, 1e-4f);
	EXPECT_NEAR(cam.Forward().z, 0.0f, 1e-4f);
}

TEST(Camera_Rotate)
{
	Camera cam;
	cam.SetRotation(0.0f, 0.0f); // forward = (1, 0, 0)
	cam.Rotate(90.0f, 0.0f);     // yaw=90 → forward = (0, 0, 1)
	EXPECT_NEAR(cam.Forward().z, 1.0f, 1e-4f);
	EXPECT_NEAR(cam.Forward().x, 0.0f, 1e-4f);
}

TEST(Camera_PitchClamp)
{
	Camera cam;
	cam.SetRotation(0.0f, 100.0f);
	EXPECT_NEAR(cam.Pitch(), 89.0f, 1e-4f);
	cam.SetRotation(0.0f, -100.0f);
	EXPECT_NEAR(cam.Pitch(), -89.0f, 1e-4f);
}

TEST(Camera_MoveForward)
{
	Camera cam;
	cam.SetRotation(0.0f, 0.0f); // forward = (1, 0, 0)
	glm::vec3 before = cam.Position();
	cam.MoveForward(5.0f);
	glm::vec3 after = cam.Position();
	EXPECT_NEAR(after.x - before.x, 5.0f, 1e-4f);
}

TEST(Camera_MoveRight)
{
	Camera cam;
	cam.SetRotation(0.0f, 0.0f); // right = cross((1,0,0),(0,1,0)) = (0, 0, 1)
	glm::vec3 before = cam.Position();
	cam.MoveRight(3.0f);
	glm::vec3 after = cam.Position();
	EXPECT_NEAR(after.z - before.z, 3.0f, 1e-4f);
}

TEST(Camera_MoveUp)
{
	Camera cam;
	glm::vec3 before = cam.Position();
	cam.MoveUp(7.0f);
	EXPECT_NEAR(cam.Position().y - before.y, 7.0f, 1e-4f);
}

TEST(Camera_PerspectiveMatrix)
{
	Camera cam;
	cam.SetPerspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	const glm::mat4& proj = cam.ProjectionMatrix();
	EXPECT_NEAR(proj[0][0], 1.0f / (std::tan(glm::radians(30.0f)) * (16.0f / 9.0f)), 1e-4f);
}

TEST(Camera_ViewMatrixConsistency)
{
	Camera cam;
	cam.SetPosition(glm::vec3(5, 0, 0));
	cam.SetRotation(0.0f, 0.0f);
	const glm::mat4& view = cam.ViewMatrix();
	// View = lookAt, camera at (5,0,0) looking along +X:
	// origin in view space should be behind camera
	glm::vec4 originInView = view * glm::vec4(5, 0, 0, 1);
	EXPECT_NEAR(originInView.x, 0.0f, 1e-4f);
	EXPECT_NEAR(originInView.z, 0.0f, 1e-4f);
}

TEST(Camera_YawGetter)
{
	Camera cam;
	cam.SetRotation(45.0f, 0.0f);
	EXPECT_NEAR(cam.Yaw(), 45.0f, 1e-4f);
}

TEST(Camera_PitchGetter)
{
	Camera cam;
	cam.SetRotation(0.0f, 30.0f);
	EXPECT_NEAR(cam.Pitch(), 30.0f, 1e-4f);
}

TEST(Camera_InitialViewMatrix)
{
	Camera cam;
	// Default: yaw=-90, pitch=0, position (0,0,3)
	// forward = (0, 0, -1), up = (0, 1, 0)
	const glm::mat4& view = cam.ViewMatrix();
	// In view space, world origin should be at (0,0,-3)
	glm::vec4 origin = view * glm::vec4(0, 0, 0, 1);
	EXPECT_NEAR(origin.z, -3.0f, 1e-4f);
}
