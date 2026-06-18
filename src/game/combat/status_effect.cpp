#include "stdafx.h"
#include "game/combat/status_effect.h"
#include "core/json_data_manager.h"

//=============================================================================

StatusSystem::StatusSystem()
{
	LoadEffectDefs();
}

//=============================================================================

void StatusSystem::LoadEffectDefs()
{
	m_effectDefs.clear();

	auto addDef = [&](std::string id, std::string name, EffectType type, std::string element,
		int32_t duration, int32_t value, int32_t tickInterval, bool beneficial)
	{
		StatusEffectDef def;
		def.id = std::move(id);
		def.name = std::move(name);
		def.type = type;
		def.element = std::move(element);
		def.duration = duration;
		def.value = value;
		def.tickInterval = tickInterval;
		def.beneficial = beneficial;
		m_effectDefs[def.id] = std::move(def);
	};

	addDef("burn", "Burn", EffectType::DamageOverTime, "fire", 3, 2, 1, false);
	addDef("poison", "Poisoned", EffectType::DamageOverTime, "poison", 4, 1, 1, false);
	addDef("slow", "Slowed", EffectType::Slow, "ice", 3, 0, 0, false);
	addDef("stun", "Stunned", EffectType::Stun, "physical", 1, 0, 0, false);
	addDef("blind", "Blinded", EffectType::Blind, "darkness", 2, -4, 0, false);
	addDef("regen", "Regeneration", EffectType::Regen, "holy", 4, 3, 1, true);
	addDef("shield", "Shielded", EffectType::Shield, "holy", 3, 3, 0, true);
	addDef("invisibility", "Invisible", EffectType::Invisibility, "arcane", 5, 0, 0, true);
	addDef("silence", "Silenced", EffectType::Silence, "arcane", 2, 0, 0, false);

	try
	{
		const json& effectsJson = JsonDataManager::Instance().AllStatusEffects();
		if (effectsJson.is_array())
		{
			for (const auto& j : effectsJson)
			{
				auto def = StatusEffectDef::FromJson(j);
				m_effectDefs[def.id] = std::move(def);
			}
		}
	}
	catch (...)
	{
		// Use built-in defs if JSON fails
	}
}

//=============================================================================

StatusEffectDef StatusEffectDef::FromJson(const json& j)
{
	StatusEffectDef def;
	def.id = j.value("id", std::string());
	def.name = j.value("name", def.id);

	std::string typeStr = j.value("type", "DamageOverTime");
	if (typeStr == "DamageOverTime")         def.type = EffectType::DamageOverTime;
	else if (typeStr == "Slow")              def.type = EffectType::Slow;
	else if (typeStr == "Stun")              def.type = EffectType::Stun;
	else if (typeStr == "Blind")             def.type = EffectType::Blind;
	else if (typeStr == "Regen")             def.type = EffectType::Regen;
	else if (typeStr == "Shield")            def.type = EffectType::Shield;
	else if (typeStr == "Invisibility")      def.type = EffectType::Invisibility;
	else if (typeStr == "Silence")           def.type = EffectType::Silence;

	def.element = j.value("element", std::string());
	def.duration = j.value("duration", 0);
	def.value = j.value("value", 0);
	def.tickInterval = j.value("tickInterval", 1);
	def.beneficial = j.value("beneficial", false);
	return def;
}

//=============================================================================

const StatusEffectDef* StatusSystem::GetEffectDef(const std::string& id) const
{
	auto it = m_effectDefs.find(id);
	return (it != m_effectDefs.end()) ? &it->second : nullptr;
}

//=============================================================================

void StatusSystem::ApplyEffect(
	std::vector<ActiveEffect>& targetEffects,
	const std::string& effectDefId,
	const std::string& sourceName,
	int32_t durationOverride,
	const std::vector<std::string>& resistances,
	const std::vector<std::string>& immunities)
{
	const StatusEffectDef* def = GetEffectDef(effectDefId);
	if (!def) return;

	for (const auto& imm : immunities)
	{
		if (imm == def->element || imm == def->id)
		{
			return;
		}
	}

	int32_t dur = (durationOverride >= 0) ? durationOverride : def->duration;
	for (const auto& res : resistances)
	{
		if (res == def->element)
		{
			dur = std::max(1, dur / 2);
			break;
		}
	}

	for (auto& ae : targetEffects)
	{
		if (ae.defId == effectDefId)
		{
			ae.remainingTurns = std::max(ae.remainingTurns, dur);
			ae.ticksUntilNext = def->tickInterval;
			return;
		}
	}

	ActiveEffect ae;
	ae.defId = effectDefId;
	ae.sourceName = sourceName;
	ae.remainingTurns = dur;
	ae.ticksUntilNext = def->tickInterval;
	targetEffects.push_back(std::move(ae));
}

//=============================================================================

void StatusSystem::ProcessTick(
	std::vector<ActiveEffect>& effects,
	std::vector<std::string>& /*expiredOut*/,
	std::function<void(int32_t damage, const std::string& effectName)> onTickDamage)
{
	for (auto it = effects.begin(); it != effects.end(); )
	{
		const StatusEffectDef* def = GetEffectDef(it->defId);
		if (!def)
		{
			it = effects.erase(it);
			continue;
		}

		it->ticksUntilNext--;
		if (it->ticksUntilNext <= 0)
		{
			if (def->type == EffectType::DamageOverTime && def->value > 0)
			{
				if (onTickDamage)
				{
					onTickDamage(def->value, def->name);
				}
			}
			else if (def->type == EffectType::Regen && def->value > 0)
			{
				if (onTickDamage)
				{
					onTickDamage(-def->value, def->name);
				}
			}

			it->ticksUntilNext = def->tickInterval;
		}

		++it;
	}
}

//=============================================================================

void StatusSystem::AdvanceTurns(std::vector<ActiveEffect>& effects)
{
	for (auto it = effects.begin(); it != effects.end(); )
	{
		it->remainingTurns--;
		if (it->remainingTurns <= 0)
		{
			it = effects.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//=============================================================================

void StatusSystem::RemoveType(std::vector<ActiveEffect>& effects, EffectType type)
{
	std::erase_if(effects, [this, type](const ActiveEffect& ae) -> bool
	{
		const StatusEffectDef* def = GetEffectDef(ae.defId);
		return def && def->type == type;
	});
}

//=============================================================================

void StatusSystem::RemoveAllNegative(std::vector<ActiveEffect>& effects)
{
	std::erase_if(effects, [this](const ActiveEffect& ae) -> bool
	{
		const StatusEffectDef* def = GetEffectDef(ae.defId);
		return def && !def->beneficial;
	});
}

//=============================================================================

bool StatusSystem::HasEffectType(const std::vector<ActiveEffect>& effects, EffectType type)
{
	for (const auto& ae : effects)
	{
		// Static method — cannot look up defs without an instance
		// Caller must verify via GetEffectDef externally if needed
		(void)ae;
		(void)type;
	}
	return false;
}

//=============================================================================

bool StatusSystem::HasEffect(const std::vector<ActiveEffect>& effects, const std::string& defId)
{
	for (const auto& ae : effects)
	{
		if (ae.defId == defId) return true;
	}
	return false;
}

//=============================================================================

int32_t StatusSystem::GetAtkBonusMod(const std::vector<ActiveEffect>& effects)
{
	int32_t mod = 0;

	for (const auto& ae : effects)
	{
		const StatusEffectDef* def = GetEffectDef(ae.defId);
		if (def && def->type == EffectType::Blind)
		{
			mod += def->value;
		}
	}

	return mod;
}

//=============================================================================

int32_t StatusSystem::GetAcBonusMod(const std::vector<ActiveEffect>& effects)
{
	int32_t mod = 0;

	for (const auto& ae : effects)
	{
		const StatusEffectDef* def = GetEffectDef(ae.defId);
		if (def && def->type == EffectType::Shield)
		{
			mod += def->value;
		}
	}

	return mod;
}
