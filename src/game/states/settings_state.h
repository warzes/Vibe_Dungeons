#pragma once

#include "engine/game_state.h"

class GameStateMachine;

struct PlayerConfig
{
	std::string playerName = "Player1";
	float mouseSensitivity = 0.5f;
	float soundVolume = 0.8f;
	int32_t resolutionWidth = 1280;
	int32_t resolutionHeight = 720;
	int32_t renderHeight = 480; //< retro FBO height; width = height * aspect
	bool fullscreen = false;
};

[[nodiscard]] int32_t GetRenderHeightFromConfig() noexcept;

class SettingsState final : public GameState
{
public:
	SettingsState(GameStateMachine& machine) noexcept;

	void OnEnter() noexcept override;
	void OnExit() noexcept override;
	void HandleEvent(const SDL_Event& event) noexcept override;
	void Update(const DeltaTime& dt) noexcept override;
	void Render() noexcept override;

	[[nodiscard]] std::string_view GetName() const noexcept override
	{
		return "Settings";
	}

private:
	GameStateMachine& m_machine;
	PlayerConfig m_config;

	[[nodiscard]] bool saveConfig() noexcept;
	[[nodiscard]] bool loadConfig() noexcept;
};
