#include "stdafx.h"
#include "game/states/main_menu_state.h"
#include "engine/delta_time.h"

MainMenuState::MainMenuState(GameStateMachine& machine) noexcept
	: m_machine(machine)
{}

void MainMenuState::OnEnter() noexcept
{
	SDL_ShowCursor();
}

void MainMenuState::OnExit() noexcept
{}

void MainMenuState::HandleEvent(const SDL_Event& /*event*/) noexcept
{}

void MainMenuState::Update(const DeltaTime& /*dt*/) noexcept
{}

void MainMenuState::Render() noexcept
{
	ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button("Start Game"))
	{
		m_machine.ReplaceState("ClassSelection");
	}

	ImGui::SameLine();

	if (ImGui::Button("Settings"))
	{
		m_machine.PushState("Settings");
	}

	ImGui::SameLine();

	if (ImGui::Button("Exit"))
	{
		SDL_Event quitEvent = {};
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
	}

	ImGui::Separator();

	if (ImGui::CollapsingHeader("JSON Serialization Example"))
	{
		ImGui::TextWrapped("See JSON example in the Settings state.");
	}

	ImGui::End();
}
