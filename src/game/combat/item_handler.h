#pragma once

#include <vector>

#include "game/combat/item.h"
#include "game/combat/item_renderer.h"

class Character;
class CombatLog;
class TurnManager;
class Camera;
class ResourceManager;
class Shader;
class Renderer;

class ItemHandler final
{
public:
	void Init(
		Character& character,
		CombatLog& combatLog,
		TurnManager& turnManager,
		Camera& camera,
		ResourceManager& resources,
		Shader& dungeonShader
	) noexcept;

	void SpawnDefault() noexcept;
	void ProcessPickup() noexcept;
	void SubmitRender(Renderer& renderer) noexcept;

	[[nodiscard]] const std::vector<ItemDrop>& Drops() const noexcept { return m_itemDrops; }
	[[nodiscard]] std::vector<ItemDrop>& Drops() noexcept { return m_itemDrops; }
	void ClearDrops() noexcept { m_itemDrops.clear(); }

private:
	Character* m_character = nullptr;
	CombatLog* m_combatLog = nullptr;
	TurnManager* m_turnManager = nullptr;
	Camera* m_camera = nullptr;
	ResourceManager* m_resources = nullptr;
	Shader* m_dungeonShader = nullptr;

	ItemRenderer m_itemRenderer;
	std::vector<ItemDrop> m_itemDrops;
};
