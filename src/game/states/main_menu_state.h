#pragma once

#include "engine/game_state.h"
#include "game/combat/character.h"

class Window;
class InputManager;

class MainMenuState final : public GameState
{
public:
	explicit MainMenuState(GameStateMachine& machine, Character* pendingCharacter = nullptr) noexcept;

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
	void loadGameFromSlot(int32_t slot) noexcept;

	GameStateMachine& m_machine;
	Character* m_pendingCharacter;
	bool m_showCredits = false;
	bool m_showLoadSlots = false;
	int32_t m_selectedSlot = -1;
};
