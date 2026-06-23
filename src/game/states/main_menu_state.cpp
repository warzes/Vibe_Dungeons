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
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 ws = io.DisplaySize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 200.0f, ws.y * 0.5f - 200.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Always);

	ImGui::Begin("Main Menu", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	ImGui::SetWindowFontScale(2.0f);
	ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.1f, 1.0f), "DUNGEON CRAWLERS");
	ImGui::SetWindowFontScale(1.0f);

	ImGui::Dummy(ImVec2(0.0f, 40.0f));

	if (ImGui::Button("Start New Game", ImVec2(300, 50)))
	{
		m_machine.ReplaceState("ClassSelection");
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (ImGui::Button("Load Game", ImVec2(300, 50)))
	{
		m_showLoadSlots = !m_showLoadSlots;
	}

	if (m_showLoadSlots)
	{
		ImGui::Separator();
		ImGui::Text("Select save slot:");
		for (int32_t s = 0; s < 3; ++s)
		{
			char label[32];
			snprintf(label, sizeof(label), "Slot %d", s + 1);
			if (ImGui::Button(label, ImVec2(300, 30)))
			{
				m_selectedSlot = s;
				m_showLoadSlots = false;
			}
		}
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (ImGui::Button("Options", ImVec2(300, 50)))
	{
		m_machine.PushState("Settings");
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (ImGui::Button("Credits", ImVec2(300, 50)))
	{
		m_showCredits = !m_showCredits;
	}

	ImGui::Dummy(ImVec2(0.0f, 40.0f));

	if (ImGui::Button("Exit", ImVec2(300, 40)))
	{
		SDL_Event quitEvent = {};
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
	}

	if (m_showCredits)
	{
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "A Vibe Dungeons Production");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Built with C++26, SDL3, OpenGL 3.3");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Inspired by classic dungeon crawlers");
	}

	ImGui::End();
}
