#include "stdafx.h"
#include "engine/renderer/camera.h"

void Camera::SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) noexcept
{
	m_fov = fovDegrees;
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	m_projection = glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
}

void Camera::SetAspectRatio(float aspectRatio) noexcept
{
	m_projection = glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::SetPosition(const glm::vec3& position) noexcept
{
	m_position = position;
	UpdateViewMatrix();
}

void Camera::SetRotation(float yaw, float pitch) noexcept
{
	m_yaw = yaw;
	m_pitch = pitch;
	updateFromYawPitch();
}

void Camera::MoveForward(float amount) noexcept
{
	m_position += m_forward * amount;
	UpdateViewMatrix();
}

void Camera::MoveRight(float amount) noexcept
{
	m_position += m_right * amount;
	UpdateViewMatrix();
}

void Camera::MoveUp(float amount) noexcept
{
	m_position += m_up * amount;
	UpdateViewMatrix();
}

void Camera::Rotate(float yawDelta, float pitchDelta) noexcept
{
	m_yaw += yawDelta;
	m_pitch += pitchDelta;
	updateFromYawPitch();
}

void Camera::UpdateViewMatrix() noexcept
{
	m_view = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::updateFromYawPitch() noexcept
{
	if (m_pitch > PITCH_MAX) m_pitch = PITCH_MAX;
	if (m_pitch < PITCH_MIN) m_pitch = PITCH_MIN;

	// Spherical coordinates
	glm::vec3 forward;
	forward.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	forward.y = std::sin(glm::radians(m_pitch));
	forward.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	m_forward = glm::normalize(forward);

	// Choose reference up based on forward direction to avoid gimbal lock
	const glm::vec3 refUp = (glm::abs(m_forward.y) > 0.999f)
		? glm::vec3(0.0f, 0.0f, 1.0f)
		: WORLD_UP;
	m_right = glm::normalize(glm::cross(m_forward, refUp));
	m_up = glm::normalize(glm::cross(m_right, m_forward));

	UpdateViewMatrix();
}

//=============================================================================
// Grid movement
//=============================================================================

glm::vec3 Camera::GridToWorld(GridPosition p) noexcept
{
	return glm::vec3(
		static_cast<float>(p.col) * GRID_UNIT + GRID_UNIT * 0.5f,
		0.5f,
		static_cast<float>(p.row) * GRID_UNIT + GRID_UNIT * 0.5f
	);
}

void Camera::SetGridPosition(GridPosition pos, Direction facing) noexcept
{
	m_gridPosition = pos;
	m_gridFacing = facing;
}

void Camera::SnapToGrid() noexcept
{
	m_position = GridToWorld(m_gridPosition);
	m_yaw = DirectionToYaw(m_gridFacing);
	m_pitch = 0.0f;
	m_isAnimating = false;
	m_animationTimer = 0.0f;
	updateFromYawPitch();
}

void Camera::TurnLeft() noexcept
{
	if (m_isAnimating)
	{
		return;
	}

	m_animStartYaw = m_yaw;
	m_animStartPosition = m_position;
	m_gridFacing = NextDirection(m_gridFacing, false);
	m_targetYaw = DirectionToYaw(m_gridFacing);
	m_animationTimer = 0.0f;
	m_isAnimating = true;
}

void Camera::TurnRight() noexcept
{
	if (m_isAnimating)
	{
		return;
	}

	m_animStartYaw = m_yaw;
	m_animStartPosition = m_position;
	m_gridFacing = NextDirection(m_gridFacing, true);
	m_targetYaw = DirectionToYaw(m_gridFacing);
	m_animationTimer = 0.0f;
	m_isAnimating = true;
}

void Camera::MoveForward() noexcept
{
	if (m_isAnimating)
	{
		return;
	}

	glm::ivec2 delta = DirectionToVec(m_gridFacing);
	GridPosition target(
		m_gridPosition.row + delta.x,
		m_gridPosition.col + delta.y,
		m_gridPosition.floor
	);

	m_animStartPosition = m_position;
	m_animStartYaw = m_yaw;
	m_gridPosition = target;
	m_targetPosition = GridToWorld(target);
	m_targetYaw = DirectionToYaw(m_gridFacing);
	m_animationTimer = 0.0f;
	m_isAnimating = true;
}

void Camera::MoveBackward() noexcept
{
	if (m_isAnimating)
	{
		return;
	}

	glm::ivec2 delta = DirectionToVec(m_gridFacing);
	GridPosition target(
		m_gridPosition.row - delta.x,
		m_gridPosition.col - delta.y,
		m_gridPosition.floor
	);

	m_animStartPosition = m_position;
	m_animStartYaw = m_yaw;
	m_gridPosition = target;
	m_targetPosition = GridToWorld(target);
	m_targetYaw = DirectionToYaw(m_gridFacing);
	m_animationTimer = 0.0f;
	m_isAnimating = true;
}

void Camera::UpdateAnimation(float dt) noexcept
{
	if (!m_isAnimating)
	{
		return;
	}

	m_animationTimer += dt;
	const float t = std::min(m_animationTimer / ANIMATION_DURATION, 1.0f);

	// Smooth step interpolation
	const float smooth = t * t * (3.0f - 2.0f * t);

	m_position = glm::mix(m_animStartPosition, m_targetPosition, smooth);
	m_yaw = glm::mix(m_animStartYaw, m_targetYaw, smooth);

	updateFromYawPitch();

	if (t >= 1.0f)
	{
		SnapToGrid();
	}
}
