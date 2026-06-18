#pragma once

#include "core/json_alias.h"
#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/item.h"
#include "game/combat/inventory.h"
#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "game/dungeon/chunk.h"

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

// ---- EquipmentSlot ----
inline void to_json(json& j, EquipmentSlot s)
{
	j = static_cast<uint8_t>(s);
}

inline void from_json(const json& j, EquipmentSlot& s)
{
	s = static_cast<EquipmentSlot>(j.get<uint8_t>());
}

// ---- Item ----
inline void to_json(json& j, const Item& item)
{
	j = json{
		{"itemId", item.itemId},
		{"name", item.name},
		{"type", item.type},
		{"rarity", item.rarity},
		{"value", item.value},
		{"bonus", item.bonus},
		{"slot", item.slot},
		{"damageMin", item.damageMin},
		{"damageMax", item.damageMax},
		{"ac", item.ac},
		{"atkBonus", item.atkBonus},
		{"strBonus", item.strBonus},
		{"dexBonus", item.dexBonus},
		{"conBonus", item.conBonus},
		{"hpBonus", item.hpBonus},
		{"mpBonus", item.mpBonus},
		{"spellId", item.spellId},
		{"charges", item.charges},
		{"maxCharges", item.maxCharges},
		{"maxDurability", item.maxDurability},
		{"durability", item.durability},
		{"sharpnessLevel", item.sharpnessLevel},
		{"prefixId", item.prefixId},
		{"postfixId", item.postfixId},
		{"materialId", item.materialId}
	};
}

inline void from_json(const json& j, Item& item)
{
	if (j.contains("itemId")) { j.at("itemId").get_to(item.itemId); }
	j.at("name").get_to(item.name);
	j.at("type").get_to(item.type);
	j.at("rarity").get_to(item.rarity);
	j.at("value").get_to(item.value);
	j.at("bonus").get_to(item.bonus);
	if (j.contains("slot")) { j.at("slot").get_to(item.slot); }
	if (j.contains("damageMin")) { j.at("damageMin").get_to(item.damageMin); }
	if (j.contains("damageMax")) { j.at("damageMax").get_to(item.damageMax); }
	if (j.contains("ac")) { j.at("ac").get_to(item.ac); }
	if (j.contains("atkBonus")) { j.at("atkBonus").get_to(item.atkBonus); }
	if (j.contains("strBonus")) { j.at("strBonus").get_to(item.strBonus); }
	if (j.contains("dexBonus")) { j.at("dexBonus").get_to(item.dexBonus); }
	if (j.contains("conBonus")) { j.at("conBonus").get_to(item.conBonus); }
	if (j.contains("hpBonus")) { j.at("hpBonus").get_to(item.hpBonus); }
	if (j.contains("mpBonus")) { j.at("mpBonus").get_to(item.mpBonus); }
	if (j.contains("spellId")) { j.at("spellId").get_to(item.spellId); }
	if (j.contains("charges")) { j.at("charges").get_to(item.charges); }
	if (j.contains("maxCharges")) { j.at("maxCharges").get_to(item.maxCharges); }
	if (j.contains("maxDurability")) { j.at("maxDurability").get_to(item.maxDurability); }
	if (j.contains("durability")) { j.at("durability").get_to(item.durability); }
	if (j.contains("sharpnessLevel")) { j.at("sharpnessLevel").get_to(item.sharpnessLevel); }
	if (j.contains("prefixId")) { j.at("prefixId").get_to(item.prefixId); }
	if (j.contains("postfixId")) { j.at("postfixId").get_to(item.postfixId); }
	if (j.contains("materialId")) { j.at("materialId").get_to(item.materialId); }
}

// ---- ItemDrop ----
void to_json(json& j, const ItemDrop& drop);
void from_json(const json& j, ItemDrop& drop);

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
		{"xpReward", m.xpReward},
		{"groupId", m.groupId},
		{"leaderId", m.leaderId},
		{"range", m.range},
		{"hasRangedAttack", m.hasRangedAttack},
		{"rangedDamageMin", m.rangedDamageMin},
		{"rangedDamageMax", m.rangedDamageMax}
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
	j.at("groupId").get_to(m.groupId);
	j.at("leaderId").get_to(m.leaderId);
	j.at("range").get_to(m.range);
	j.at("hasRangedAttack").get_to(m.hasRangedAttack);
	j.at("rangedDamageMin").get_to(m.rangedDamageMin);
	j.at("rangedDamageMax").get_to(m.rangedDamageMax);
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

// ---- Equipment ----
inline void to_json(json& j, const Equipment& eq)
{
	eq.to_json(j);
}

inline void from_json(const json& j, Equipment& eq)
{
	eq.from_json(j);
}

// ---- Character (with inventory) ----
inline void to_json(json& j, const Character& c)
{
	json invJson = json::array();
	for (size_t i = 0; i < c.m_inventory.Size(); ++i)
	{
		const Item* item = c.m_inventory.Get(i);
		if (item)
		{
			invJson.push_back(*item);
		}
	}

	json slotsJson = json::array();
	for (const auto& slot : c.m_actionSlots)
	{
		slotsJson.push_back(slot);
	}

	j = json{
		{"name", c.m_name},
		{"charClass", c.m_charClass},
		{"level", c.m_level},
		{"hp", c.m_hp},
		{"maxHp", c.m_maxHp},
		{"mp", c.m_mp},
		{"maxMp", c.m_maxMp},
		{"ac", c.m_ac},
		{"str", c.m_str},
		{"dex", c.m_dex},
		{"con", c.m_con},
		{"intel", c.m_intel},
		{"atkBonus", c.m_atkBonus},
		{"damageBonus", c.m_damageBonus},
		{"damageMin", c.m_damageMin},
		{"damageMax", c.m_damageMax},
		{"xp", c.m_xp},
		{"xpForNext", c.m_xpForNext},
		{"position", c.m_position},
		{"facing", c.m_facing},
		{"inventory", std::move(invJson)},
		{"equipment", c.m_equipment},
		{"unlockedSkills", c.m_unlockedSkills},
		{"actionSlots", std::move(slotsJson)},
		{"learnedSpells", c.m_learnedSpells}
	};
}

inline void from_json(const json& j, Character& c)
{
	j.at("name").get_to(c.m_name);
	j.at("charClass").get_to(c.m_charClass);
	j.at("level").get_to(c.m_level);
	j.at("hp").get_to(c.m_hp);
	j.at("maxHp").get_to(c.m_maxHp);
	j.at("mp").get_to(c.m_mp);
	j.at("maxMp").get_to(c.m_maxMp);
	j.at("ac").get_to(c.m_ac);
	j.at("str").get_to(c.m_str);
	j.at("dex").get_to(c.m_dex);
	j.at("con").get_to(c.m_con);
	j.at("intel").get_to(c.m_intel);
	j.at("atkBonus").get_to(c.m_atkBonus);
	j.at("damageBonus").get_to(c.m_damageBonus);
	j.at("damageMin").get_to(c.m_damageMin);
	j.at("damageMax").get_to(c.m_damageMax);
	j.at("xp").get_to(c.m_xp);
	j.at("xpForNext").get_to(c.m_xpForNext);
	j.at("position").get_to(c.m_position);
	j.at("facing").get_to(c.m_facing);

	c.m_inventory.Clear();
	const json& invJson = j.at("inventory");
	for (const auto& itemJson : invJson)
	{
		c.m_inventory.Add(itemJson.get<Item>());
	}

	if (j.contains("equipment"))
	{
		c.m_equipment.from_json(j.at("equipment"));
	}

	c.m_unlockedSkills.clear();
	if (j.contains("unlockedSkills"))
	{
		for (const auto& s : j.at("unlockedSkills"))
		{
			c.m_unlockedSkills.push_back(s.get<std::string>());
		}
	}

	c.m_learnedSpells.clear();
	if (j.contains("learnedSpells"))
	{
		for (const auto& s : j.at("learnedSpells"))
		{
			c.m_learnedSpells.push_back(s.get<std::string>());
		}
	}

	if (j.contains("actionSlots"))
	{
		const json& slotsJson = j.at("actionSlots");
		for (size_t i = 0; i < slotsJson.size() && i < static_cast<size_t>(Character::NUM_ACTION_SLOTS); ++i)
		{
			slotsJson[i].get_to(c.m_actionSlots[i]);
		}
	}
}
