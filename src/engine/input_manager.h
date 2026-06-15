#pragma once

class InputManager final
{
public:
	explicit InputManager(SDL_Window* window) noexcept;

	void BeginFrame() noexcept;
	void ProcessEvent(const SDL_Event& event) noexcept;

	// Raw key/mouse queries
	[[nodiscard]] bool IsKeyDown(SDL_Scancode scancode) const noexcept;
	[[nodiscard]] bool IsKeyPressed(SDL_Scancode scancode) const noexcept;
	[[nodiscard]] bool IsKeyReleased(SDL_Scancode scancode) const noexcept;

	[[nodiscard]] bool IsMouseButtonDown(uint8_t button) const noexcept;
	[[nodiscard]] bool IsMouseButtonPressed(uint8_t button) const noexcept;
	[[nodiscard]] bool IsMouseButtonReleased(uint8_t button) const noexcept;

	// Action mapping
	void BindAction(std::string_view name, SDL_Scancode key) noexcept;
	void UnbindAction(std::string_view name) noexcept;
	void ClearActions() noexcept;

	[[nodiscard]] bool IsActionDown(std::string_view name) const noexcept;
	[[nodiscard]] bool IsActionPressed(std::string_view name) const noexcept;
	[[nodiscard]] bool IsActionReleased(std::string_view name) const noexcept;

	// Mouse state
	[[nodiscard]] float MouseDeltaX() const noexcept
	{
		return m_mouseDeltaX;
	}

	[[nodiscard]] float MouseDeltaY() const noexcept
	{
		return m_mouseDeltaY;
	}

	[[nodiscard]] float MouseX() const noexcept
	{
		return m_mouseX;
	}

	[[nodiscard]] float MouseY() const noexcept
	{
		return m_mouseY;
	}

	void ResetState() noexcept;

	void SetMouseCaptured(bool captured) noexcept;
	[[nodiscard]] bool IsMouseCaptured() const noexcept
	{
		return m_mouseCaptured;
	}

private:
	struct ActionBinding
	{
		SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
	};

	static constexpr size_t KEY_COUNT = SDL_SCANCODE_COUNT;
	static constexpr size_t MOUSE_BUTTON_COUNT = SDL_BUTTON_X2 + 1;

	std::array<bool, KEY_COUNT> m_keysDown {};
	std::array<bool, KEY_COUNT> m_keysPressed {};
	std::array<bool, KEY_COUNT> m_keysReleased {};

	std::array<bool, MOUSE_BUTTON_COUNT> m_mouseDown {};
	std::array<bool, MOUSE_BUTTON_COUNT> m_mousePressed {};
	std::array<bool, MOUSE_BUTTON_COUNT> m_mouseReleased {};

	struct transparent_hash final
	{
		using is_transparent = void;
		[[nodiscard]] size_t operator()(std::string_view sv) const noexcept
		{
			return std::hash<std::string_view>{}(sv);
		}
		[[nodiscard]] size_t operator()(const std::string& s) const noexcept
		{
			return std::hash<std::string>{}(s);
		}
	};
	std::unordered_map<std::string, ActionBinding, transparent_hash, std::equal_to<>> m_actionBindings;

	SDL_Window* m_window;
	float m_mouseX = 0.0f;
	float m_mouseY = 0.0f;
	float m_mouseDeltaX = 0.0f;
	float m_mouseDeltaY = 0.0f;
	bool m_mouseCaptured = false;
};
