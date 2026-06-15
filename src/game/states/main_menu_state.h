#pragma once

#include "engine/game_state.h"

class Window;
class InputManager;

class MainMenuState final : public GameState
{
public:
	MainMenuState(
		GameStateMachine& machine,
		const Window& window,
		InputManager& input
	) noexcept;

	void OnEnter() noexcept override;
	void OnExit() noexcept override;
	void HandleEvent(const SDL_Event& event) noexcept override;
	void Update(const DeltaTime& dt) noexcept override;
	void Render() noexcept override;

	[[nodiscard]] std::string_view GetName() const noexcept override
	{
		return "MainMenu";
	}

private:
	GameStateMachine& m_machine;
	[[maybe_unused]] const Window& m_window;
	[[maybe_unused]] InputManager& m_input;
};
