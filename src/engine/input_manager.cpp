#include "stdafx.h"
#include "engine/input_manager.h"

InputManager::InputManager(SDL_Window* window) noexcept
	: m_window(window)
{}

void InputManager::BeginFrame() noexcept
{
	m_keysPressed.fill(false);
	m_keysReleased.fill(false);
	m_mousePressed.fill(false);
	m_mouseReleased.fill(false);
	m_mouseDeltaX = 0.0f;
	m_mouseDeltaY = 0.0f;
}

void InputManager::ProcessEvent(const SDL_Event& event) noexcept
{
	switch (event.type)
	{
		case SDL_EVENT_KEY_DOWN:
		{
			if (event.key.repeat) break;
			const auto scancode = event.key.scancode;
			if (scancode < KEY_COUNT && !m_keysDown[scancode])
			{
				m_keysPressed[scancode] = true;
				m_keysDown[scancode] = true;
			}
			break;
		}
		case SDL_EVENT_KEY_UP:
		{
			const auto scancode = event.key.scancode;
			if (scancode < KEY_COUNT)
			{
				m_keysReleased[scancode] = true;
				m_keysDown[scancode] = false;
			}
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			const auto button = event.button.button;
			if (button < MOUSE_BUTTON_COUNT && !m_mouseDown[button])
			{
				m_mousePressed[button] = true;
				m_mouseDown[button] = true;
			}
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			const auto button = event.button.button;
			if (button < MOUSE_BUTTON_COUNT)
			{
				m_mouseReleased[button] = true;
				m_mouseDown[button] = false;
			}
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
		{
			if (m_mouseCaptured)
			{
				m_mouseDeltaX += event.motion.xrel;
				m_mouseDeltaY += event.motion.yrel;
			}
			m_mouseX = event.motion.x;
			m_mouseY = event.motion.y;
			break;
		}
	}
}

bool InputManager::IsKeyDown(SDL_Scancode scancode) const noexcept
{
	return scancode < KEY_COUNT && m_keysDown[scancode];
}

bool InputManager::IsKeyPressed(SDL_Scancode scancode) const noexcept
{
	return scancode < KEY_COUNT && m_keysPressed[scancode];
}

bool InputManager::IsKeyReleased(SDL_Scancode scancode) const noexcept
{
	return scancode < KEY_COUNT && m_keysReleased[scancode];
}

bool InputManager::IsMouseButtonDown(uint8_t button) const noexcept
{
	return button < MOUSE_BUTTON_COUNT && m_mouseDown[button];
}

bool InputManager::IsMouseButtonPressed(uint8_t button) const noexcept
{
	return button < MOUSE_BUTTON_COUNT && m_mousePressed[button];
}

bool InputManager::IsMouseButtonReleased(uint8_t button) const noexcept
{
	return button < MOUSE_BUTTON_COUNT && m_mouseReleased[button];
}

// ── Action mapping ────────────────────────────────────────────────────────

void InputManager::BindAction(std::string_view name, SDL_Scancode key) noexcept
{
	assert(key < KEY_COUNT && "BindAction: scancode out of range");
	ActionBinding binding;
	binding.scancode = key;
	m_actionBindings.emplace(name, binding);
}

void InputManager::UnbindAction(std::string_view name) noexcept
{
	m_actionBindings.erase(std::string(name)); // erase still needs conversion
}

void InputManager::ClearActions() noexcept
{
	m_actionBindings.clear();
}

bool InputManager::IsActionDown(std::string_view name) const noexcept
{
	auto it = m_actionBindings.find(name);
	if (it == m_actionBindings.end())
	{
		return false;
	}
	return IsKeyDown(it->second.scancode);
}

bool InputManager::IsActionPressed(std::string_view name) const noexcept
{
	auto it = m_actionBindings.find(name);
	if (it == m_actionBindings.end())
	{
		return false;
	}
	return IsKeyPressed(it->second.scancode);
}

bool InputManager::IsActionReleased(std::string_view name) const noexcept
{
	auto it = m_actionBindings.find(name);
	if (it == m_actionBindings.end())
	{
		return false;
	}
	return IsKeyReleased(it->second.scancode);
}

void InputManager::ResetState() noexcept
{
	m_keysDown.fill(false);
	m_keysPressed.fill(false);
	m_keysReleased.fill(false);
	m_mouseDown.fill(false);
	m_mousePressed.fill(false);
	m_mouseReleased.fill(false);
	m_mouseX = 0.0f;
	m_mouseY = 0.0f;
	m_mouseDeltaX = 0.0f;
	m_mouseDeltaY = 0.0f;
	m_mouseCaptured = false;
	SDL_SetWindowRelativeMouseMode(m_window, false);
	SDL_ShowCursor();
}

void InputManager::SetMouseCaptured(bool captured) noexcept
{
	m_mouseCaptured = captured;
	SDL_SetWindowRelativeMouseMode(m_window, captured);
	if (captured)
	{
		SDL_HideCursor();
	}
	else
	{
		SDL_ShowCursor();
	}
}
