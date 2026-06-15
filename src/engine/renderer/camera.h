#pragma once

class Camera final
{
public:
	Camera() noexcept
	{
		updateFromYawPitch();
	}

	void SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) noexcept;
	void SetAspectRatio(float aspectRatio) noexcept;
	void SetPosition(const glm::vec3& position) noexcept;
	void SetRotation(float yaw, float pitch) noexcept;

	void MoveForward(float amount) noexcept;
	void MoveRight(float amount) noexcept;
	void MoveUp(float amount) noexcept;
	void Rotate(float yawDelta, float pitchDelta) noexcept;

	[[nodiscard]] const glm::vec3& Position() const noexcept
	{
		return m_position;
	}

	[[nodiscard]] const glm::vec3& Forward() const noexcept
	{
		return m_forward;
	}

	[[nodiscard]] const glm::vec3& Right() const noexcept
	{
		return m_right;
	}

	[[nodiscard]] const glm::vec3& Up() const noexcept
	{
		return m_up;
	}

	[[nodiscard]] const glm::mat4& ViewMatrix() const noexcept
	{
		return m_view;
	}

	[[nodiscard]] const glm::mat4& ProjectionMatrix() const noexcept
	{
		return m_projection;
	}

	[[nodiscard]] float Yaw() const noexcept
	{
		return m_yaw;
	}

	[[nodiscard]] float Pitch() const noexcept
	{
		return m_pitch;
	}

	void UpdateViewMatrix() noexcept;

	static constexpr float PITCH_MAX = 89.0f;
	static constexpr float PITCH_MIN = -89.0f;
	static constexpr glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

private:
	void updateFromYawPitch() noexcept;

	float m_fov = 60.0f;
	float m_nearPlane = 0.1f;
	float m_farPlane = 100.0f;

	glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);

	float m_yaw = -90.0f;
	float m_pitch = 0.0f;

	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);
};
