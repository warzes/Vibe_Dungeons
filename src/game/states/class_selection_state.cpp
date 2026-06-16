#include "stdafx.h"
#include "game/states/class_selection_state.h"
#include "engine/delta_time.h"
#include "engine/window.h"
#include "engine/input_manager.h"
#include "core/json_data_manager.h"
#include "game/data/item_factory.h"
#include "game/data/experience_system.h"
#include <imgui/imgui.h>

Character ClassSelectionState::s_pendingCharacter;

ClassSelectionState::ClassSelectionState(
	GameStateMachine& machine,
	const Window& window,
	InputManager& input
) noexcept
	: m_machine(machine)
	, m_window(window)
	, m_input(input)
{}

void ClassSelectionState::OnEnter() noexcept
{
	SDL_ShowCursor();
}

void ClassSelectionState::OnExit() noexcept
{}

void ClassSelectionState::HandleEvent(const SDL_Event& /*event*/) noexcept
{}

void ClassSelectionState::Update(const DeltaTime& /*dt*/) noexcept
{}

void ClassSelectionState::Render() noexcept
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 center = viewport->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(600, 420), ImGuiCond_Always);

	ImGui::Begin("Choose Your Class", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	const json& classes = JsonDataManager::Instance().AllClasses();

	float panelWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;

	int32_t index = 0;
	for (const auto& cls : classes)
	{
		std::string id = cls.value("id", std::string());
		std::string name = cls.value("name", id);
		std::string desc = cls.value("description", std::string());

		if (index > 0 && index % 3 != 0)
		{
			ImGui::SameLine();
		}

		ImGui::BeginGroup();
		ImGui::PushID(id.c_str());

		ImVec4 panelColor = (m_selectedClass == id)
			? ImVec4(0.3f, 0.5f, 0.3f, 1.0f)
			: ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, panelColor);
		ImGui::BeginChild("##panel", ImVec2(panelWidth, 180), true);
		ImGui::TextWrapped("%s", name.c_str());
		ImGui::Separator();
		ImGui::TextWrapped("%s", desc.c_str());
		ImGui::Separator();
		ImGui::Text("STR: %d  DEX: %d  CON: %d  INT: %d",
			cls.value("baseStr", 10), cls.value("baseDex", 10),
			cls.value("baseCon", 10), cls.value("baseInt", 10));
		ImGui::Text("HP: %d  ATK: %+d  AC: %+d",
			cls.value("baseHp", 10), cls.value("atkBonus", 0), cls.value("acBonus", 0));

		if (ImGui::Button("Select", ImVec2(-1.0f, 0.0f)))
		{
			m_selectedClass = id;
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopID();
		ImGui::EndGroup();

		++index;
	}

	ImGui::Separator();

	if (ImGui::Button("Back", ImVec2(100, 0)))
	{
		m_machine.ReplaceState("MainMenu");
	}

	ImGui::SameLine();

	ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 120, 0));
	ImGui::SameLine();

	if (ImGui::Button("Confirm", ImVec2(120, 0)))
	{
		s_pendingCharacter = CreateCharacter(m_selectedClass);
		m_machine.ReplaceState("Play");
	}

	ImGui::End();
}

Character ClassSelectionState::CreateCharacter(const std::string& classId)
{
	const json& data = JsonDataManager::Instance().GetClassData(classId);

	Character c;
	c.charClass = classId;
	c.name = data.value("name", classId);
	c.level = 1;
	c.maxHp = data.value("baseHp", 20);
	c.hp = c.maxHp;
	c.maxMp = data.value("baseMp", 0);
	c.mp = c.maxMp;
	c.ac = 10 + data.value("acBonus", 0);
	c.atkBonus = data.value("atkBonus", 1);
	c.damageBonus = data.value("damageBonus", 0);
	c.str = data.value("baseStr", 10);
	c.dex = data.value("baseDex", 10);
	c.con = data.value("baseCon", 10);
	c.intel = data.value("baseInt", 10);
	c.damageMin = 1;
	c.damageMax = 6;
	c.xp = 0;
	c.xpForNext = 100;

	// Starting equipment
	if (data.contains("startingEquipment"))
	{
		for (const auto& eqId : data["startingEquipment"])
		{
			Item item = ItemFactory::CreateBase(eqId.get<std::string>());
			c.inventory.Add(std::move(item));
		}
	}

	// Starting abilities
	if (data.contains("startingAbilities"))
	{
		int32_t slotIdx = 0;
		for (const auto& abId : data["startingAbilities"])
		{
			std::string sid = abId.get<std::string>();
			c.unlockedSkills.push_back(sid);
			if (slotIdx < Character::NUM_ACTION_SLOTS)
			{
				c.actionSlots[slotIdx].type = "ability";
				c.actionSlots[slotIdx].id = sid;
				c.actionSlots[slotIdx].cooldownRemaining = 0;
				++slotIdx;
			}
		}
	}

	// Apply passive skills
	ExperienceSystem::ApplyPassiveSkills(c);
	c.hp = c.maxHp;
	c.mp = c.maxMp;

	return c;
}
