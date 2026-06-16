#pragma once

#include <nlohmann/json.hpp>
#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/item.h"
#include "game/combat/inventory.h"
#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "game/dungeon/chunk.h"

using json = nlohmann::json;

// ---- GridPosition ----
inline void to_json(json& j, const GridPosition& p)
{
	j = json{ {"row", p.row}, {"col", p.col}, {"floor", p.floor} };
}

inline void from_json(const json& j, GridPosition& p)
{
	j.at("row").get_to(p.row);
	j.at("col").get_to(p.col);
	j.at("floor").get_to(p.floor);
}

// ---- Direction ----
inline void to_json(json& j, Direction d)
{
	j = json{ {"direction", static_cast<uint8_t>(d)} };
}

inline void from_json(const json& j, Direction& d)
{
	uint8_t val;
	j.at("direction").get_to(val);
	d = static_cast<Direction>(val);
}

// ---- ItemType ----
inline void to_json(json& j, ItemType t)
{
	j = static_cast<uint8_t>(t);
}

inline void from_json(const json& j, ItemType& t)
{
	t = static_cast<ItemType>(j.get<uint8_t>());
}

// ---- ItemRarity ----
inline void to_json(json& j, ItemRarity r)
{
	j = static_cast<uint8_t>(r);
}

inline void from_json(const json& j, ItemRarity& r)
{
	r = static_cast<ItemRarity>(j.get<uint8_t>());
}

// ---- Item ----
inline void to_json(json& j, const Item& item)
{
	j = json{
		{"name", item.name},
		{"type", item.type},
		{"rarity", item.rarity},
		{"value", item.value},
		{"bonus", item.bonus}
	};
}

inline void from_json(const json& j, Item& item)
{
	j.at("name").get_to(item.name);
	j.at("type").get_to(item.type);
	j.at("rarity").get_to(item.rarity);
	j.at("value").get_to(item.value);
	j.at("bonus").get_to(item.bonus);
}

// ---- ItemDrop ----
inline void to_json(json& j, const ItemDrop& drop)
{
	j = json{
		{"item", drop.item},
		{"position", drop.position}
	};
}

inline void from_json(const json& j, ItemDrop& drop)
{
	j.at("item").get_to(drop.item);
	j.at("position").get_to(drop.position);
}

// ---- MonsterAI ----
inline void to_json(json& j, MonsterAI ai)
{
	j = static_cast<uint8_t>(ai);
}

inline void from_json(const json& j, MonsterAI& ai)
{
	ai = static_cast<MonsterAI>(j.get<uint8_t>());
}

// ---- Monster ----
inline void to_json(json& j, const Monster& m)
{
	j = json{
		{"name", m.name},
		{"typeId", m.typeId},
		{"level", m.level},
		{"hp", m.hp},
		{"maxHp", m.maxHp},
		{"ac", m.ac},
		{"atkBonus", m.atkBonus},
		{"damageMin", m.damageMin},
		{"damageMax", m.damageMax},
		{"position", m.position},
		{"facing", m.facing},
		{"ai", m.ai},
		{"alive", m.alive},
		{"xpReward", m.xpReward}
	};
}

inline void from_json(const json& j, Monster& m)
{
	j.at("name").get_to(m.name);
	j.at("typeId").get_to(m.typeId);
	j.at("level").get_to(m.level);
	j.at("hp").get_to(m.hp);
	j.at("maxHp").get_to(m.maxHp);
	j.at("ac").get_to(m.ac);
	j.at("atkBonus").get_to(m.atkBonus);
	j.at("damageMin").get_to(m.damageMin);
	j.at("damageMax").get_to(m.damageMax);
	j.at("position").get_to(m.position);
	j.at("facing").get_to(m.facing);
	j.at("ai").get_to(m.ai);
	j.at("alive").get_to(m.alive);
	j.at("xpReward").get_to(m.xpReward);
}

// ---- Cell ----
inline void to_json(json& j, const Cell& cell)
{
	j = json{
		{"hasFloor", cell.hasFloor},
		{"hasCeiling", cell.hasCeiling},
		{"isWall", cell.isWall}
	};
}

inline void from_json(const json& j, Cell& cell)
{
	j.at("hasFloor").get_to(cell.hasFloor);
	j.at("hasCeiling").get_to(cell.hasCeiling);
	j.at("isWall").get_to(cell.isWall);
}

// ---- Chunk ----
inline void to_json(json& j, const Chunk& chunk)
{
	json cellsJson = json::array();
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			json cellJson;
			to_json(cellJson, chunk.At(r, c));
			cellJson["row"] = r;
			cellJson["col"] = c;
			cellsJson.push_back(std::move(cellJson));
		}
	}
	j = json{ {"cells", std::move(cellsJson)} };
}

inline void from_json(const json& j, Chunk& chunk)
{
	const json& cellsJson = j.at("cells");
	for (const auto& cellJson : cellsJson)
	{
		int32_t r = cellJson.at("row").get<int32_t>();
		int32_t c = cellJson.at("col").get<int32_t>();
		from_json(cellJson, chunk.At(r, c));
	}
}

// ---- ActionSlot ----
inline void to_json(json& j, const ActionSlot& s)
{
	j = json{
		{"type", s.type},
		{"id", s.id},
		{"cooldownRemaining", s.cooldownRemaining}
	};
}

inline void from_json(const json& j, ActionSlot& s)
{
	j.at("type").get_to(s.type);
	j.at("id").get_to(s.id);
	j.at("cooldownRemaining").get_to(s.cooldownRemaining);
}

// ---- Character (with inventory) ----
inline void to_json(json& j, const Character& c)
{
	json invJson = json::array();
	for (size_t i = 0; i < c.inventory.Size(); ++i)
	{
		const Item* item = c.inventory.Get(i);
		if (item)
		{
			invJson.push_back(*item);
		}
	}

	json slotsJson = json::array();
	for (const auto& slot : c.actionSlots)
	{
		slotsJson.push_back(slot);
	}

	j = json{
		{"name", c.name},
		{"charClass", c.charClass},
		{"level", c.level},
		{"hp", c.hp},
		{"maxHp", c.maxHp},
		{"mp", c.mp},
		{"maxMp", c.maxMp},
		{"ac", c.ac},
		{"str", c.str},
		{"dex", c.dex},
		{"con", c.con},
		{"intel", c.intel},
		{"atkBonus", c.atkBonus},
		{"damageBonus", c.damageBonus},
		{"damageMin", c.damageMin},
		{"damageMax", c.damageMax},
		{"xp", c.xp},
		{"xpForNext", c.xpForNext},
		{"position", c.position},
		{"facing", c.facing},
		{"inventory", std::move(invJson)},
		{"unlockedSkills", c.unlockedSkills},
		{"actionSlots", std::move(slotsJson)},
		{"learnedSpells", c.learnedSpells}
	};
}

inline void from_json(const json& j, Character& c)
{
	j.at("name").get_to(c.name);
	j.at("charClass").get_to(c.charClass);
	j.at("level").get_to(c.level);
	j.at("hp").get_to(c.hp);
	j.at("maxHp").get_to(c.maxHp);
	j.at("mp").get_to(c.mp);
	j.at("maxMp").get_to(c.maxMp);
	j.at("ac").get_to(c.ac);
	j.at("str").get_to(c.str);
	j.at("dex").get_to(c.dex);
	j.at("con").get_to(c.con);
	j.at("intel").get_to(c.intel);
	j.at("atkBonus").get_to(c.atkBonus);
	j.at("damageBonus").get_to(c.damageBonus);
	j.at("damageMin").get_to(c.damageMin);
	j.at("damageMax").get_to(c.damageMax);
	j.at("xp").get_to(c.xp);
	j.at("xpForNext").get_to(c.xpForNext);
	j.at("position").get_to(c.position);
	j.at("facing").get_to(c.facing);

	c.inventory.Clear();
	const json& invJson = j.at("inventory");
	for (const auto& itemJson : invJson)
	{
		c.inventory.Add(itemJson.get<Item>());
	}

	c.unlockedSkills.clear();
	if (j.contains("unlockedSkills"))
	{
		for (const auto& s : j.at("unlockedSkills"))
		{
			c.unlockedSkills.push_back(s.get<std::string>());
		}
	}

	c.learnedSpells.clear();
	if (j.contains("learnedSpells"))
	{
		for (const auto& s : j.at("learnedSpells"))
		{
			c.learnedSpells.push_back(s.get<std::string>());
		}
	}

	if (j.contains("actionSlots"))
	{
		const json& slotsJson = j.at("actionSlots");
		for (size_t i = 0; i < slotsJson.size() && i < static_cast<size_t>(Character::NUM_ACTION_SLOTS); ++i)
		{
			slotsJson[i].get_to(c.actionSlots[i]);
		}
	}
}
