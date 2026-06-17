#pragma once

#include "engine/game_state.h"
#include "core/json_data_manager.h"
#include "game/combat/character.h"

class Window;
class InputManager;

[[nodiscard]] Character CreateCharacterFromClass(const std::string& classId);

class ClassSelectionState final : public GameState
{
public:
	ClassSelectionState(
		GameStateMachine& machine,
		const Window& window,
		InputManager& input,
		Character& characterOut
	) noexcept;

	void OnEnter() noexcept override;
	void OnExit() noexcept override;
	void HandleEvent(const SDL_Event& event) noexcept override;
	void Update(const DeltaTime& dt) noexcept override;
	void Render() noexcept override;

	[[nodiscard]] std::string_view GetName() const noexcept override
	{
		return "ClassSelection";
	}

private:

	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	Character* m_characterOut = nullptr;
	std::string m_selectedClass = "barbarian";
};
