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
