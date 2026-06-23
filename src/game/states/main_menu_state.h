#pragma once

#include "engine/game_state.h"

class Window;
class InputManager;

class MainMenuState final : public GameState
{
public:
	explicit MainMenuState(GameStateMachine& machine) noexcept;

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
	bool m_showCredits = false;
	bool m_showLoadSlots = false;
	int32_t m_selectedSlot = -1;
};
