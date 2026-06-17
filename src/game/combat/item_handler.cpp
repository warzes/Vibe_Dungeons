#include "stdafx.h"
#include "game/combat/item_handler.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/camera.h"
#include "game/combat/character.h"
#include "game/ui/combat_log.h"
#include "game/turn_manager.h"
#include "game/data/item_factory.h"
#include "game/grid_position.h"
#include "game/direction.h"

//=============================================================================

void ItemHandler::Init(
	Character& character,
	CombatLog& combatLog,
	TurnManager& turnManager,
	Camera& camera,
	ResourceManager& resources,
	Shader& dungeonShader
) noexcept
{
	m_character = &character;
	m_combatLog = &combatLog;
	m_turnManager = &turnManager;
	m_camera = &camera;
	m_resources = &resources;
	m_dungeonShader = &dungeonShader;

	auto loadOrMakeTex = [&](const char* key, const char* file,
		int r1, int g1, int b1, int r2, int g2, int b2) -> Texture*
	{
		Texture* tex = m_resources->LoadTexture(key, file, false);
		if (!tex)
		{
			tex = m_resources->CreateCheckerboard(
				(std::string(key) + "_cb").c_str(), 32, 4,
				r1, g1, b1, 255, r2, g2, b2, 255);
		}
		return tex;
	};

	Texture* texHeal   = loadOrMakeTex("tex_item_heal",   "data/item_potion_heal.png", 255, 60, 60, 180, 0, 0);
	Texture* texMana   = loadOrMakeTex("tex_item_mana",   "data/item_potion_mana.png", 60, 120, 255, 0, 40, 180);
	Texture* texKey    = loadOrMakeTex("tex_item_key",    "data/item_key.png",          255, 215, 0, 180, 140, 0);
	Texture* texGold   = loadOrMakeTex("tex_item_gold",   "data/item_gold.png",         255, 200, 50, 200, 140, 0);
	Texture* texScroll = loadOrMakeTex("tex_item_scroll", "data/item_scroll.png",       240, 230, 200, 200, 180, 140);
	Texture* texSword  = loadOrMakeTex("tex_item_sword",  "data/item_sword.png",        180, 180, 200, 100, 100, 130);
	Texture* texArmor  = loadOrMakeTex("tex_item_armor",  "data/item_armor.png",        120, 120, 140, 70, 70, 90);
	Texture* texShield = loadOrMakeTex("tex_item_shield", "data/item_shield.png",       160, 120, 80, 110, 80, 50);
	Texture* texQuest  = loadOrMakeTex("tex_item_quest",  "data/item_quest.png",        180, 60, 200, 120, 20, 140);

	if (!texHeal || !texKey || !texGold)
	{
		return;
	}

	auto makeMat = [&](const char* key, Texture& tex) -> Material*
	{
		return m_resources->GetOrCreateMaterial(key, *m_dungeonShader, tex);
	};

	Material* matHeal   = makeMat("mat_item_heal",   *texHeal);
	Material* matMana   = makeMat("mat_item_mana",   *texMana);
	Material* matKey    = makeMat("mat_item_key",    *texKey);
	Material* matGold   = makeMat("mat_item_gold",   *texGold);
	Material* matScroll = makeMat("mat_item_scroll", *texScroll);
	Material* matSword  = makeMat("mat_item_sword",  *texSword);
	Material* matArmor  = makeMat("mat_item_armor",  *texArmor);
	Material* matShield = makeMat("mat_item_shield", *texShield);
	Material* matQuest  = makeMat("mat_item_quest",  *texQuest);

	if (!matHeal || !matKey || !matGold)
	{
		return;
	}

	m_itemRenderer.Init();
	m_itemRenderer.SetMaterial(ItemType::PotionHeal,  *matHeal);
	m_itemRenderer.SetMaterial(ItemType::PotionMana,  *matMana);
	m_itemRenderer.SetMaterial(ItemType::Key,          *matKey);
	m_itemRenderer.SetMaterial(ItemType::Gold,         *matGold);
	m_itemRenderer.SetMaterial(ItemType::Scroll,       *matScroll);
	m_itemRenderer.SetMaterial(ItemType::Weapon,       *matSword);
	m_itemRenderer.SetMaterial(ItemType::Armor,        *matArmor);
	m_itemRenderer.SetMaterial(ItemType::Shield,       *matShield);
	m_itemRenderer.SetMaterial(ItemType::QuestItem,    *matQuest);
}

//=============================================================================

void ItemHandler::SpawnDefault() noexcept
{
	{
		ItemDrop drop;
		drop.item = ItemFactory::CreatePotion("heal", 10);
		drop.position = {16, 17, 0};
		m_itemDrops.push_back(std::move(drop));
	}
	{
		ItemDrop drop;
		drop.item = ItemFactory::CreateGold(25);
		drop.position = {14, 17, 0};
		m_itemDrops.push_back(std::move(drop));
	}
}

//=============================================================================

void ItemHandler::ProcessPickup() noexcept
{
	GridPosition heroPos = m_camera->GetGridPosition();

	glm::ivec2 fwdDelta = DirectionToVec(m_camera->Facing());
	GridPosition frontPos(
		heroPos.row + fwdDelta.x,
		heroPos.col + fwdDelta.y,
		heroPos.floor
	);

	GridPosition checkPositions[] = {heroPos, frontPos};

	for (const GridPosition& checkPos : checkPositions)
	{
		for (auto it = m_itemDrops.begin(); it != m_itemDrops.end(); ++it)
		{
			if (it->position.row == checkPos.row
				&& it->position.col == checkPos.col
				&& it->position.floor == checkPos.floor)
			{
				if (m_character->GetInventory().Add(it->item) == AddResult::Success)
				{
					std::string msg = "Picked up " + it->item.name + "!";
					m_combatLog->Add(msg, glm::vec3(0.2f, 0.8f, 1.0f));
					m_itemDrops.erase(it);
					m_turnManager->SetGameMode(GameMode::TurnWaiting);
					return;
				}
				else
				{
					m_combatLog->Add("Inventory full!", glm::vec3(1.0f, 0.5f, 0.0f));
					return;
				}
			}
		}
	}

	m_combatLog->Add("Nothing to attack or pick up.", glm::vec3(0.6f));
}

//=============================================================================

void ItemHandler::SubmitRender(Renderer& renderer) noexcept
{
	m_itemRenderer.Submit(renderer, *m_camera, m_itemDrops);
}
