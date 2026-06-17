#include "stdafx.h"
#include "game/turn_manager.h"
#include "game/combat/character.h"
#include "core/json_data_manager.h"

//=============================================================================

void TurnManager::EndPlayerTurn(bool isAnimating) noexcept
{
	AdvanceTurn();

	if (isAnimating)
	{
		m_gameMode = GameMode::TurnAnimating;
	}
	else
	{
		m_gameMode = GameMode::Exploring;
	}
}

//=============================================================================

void TurnManager::ApplyRegen(Character& character) const noexcept
{
	const json& classData = JsonDataManager::Instance().GetClassData(character.GetClass());
	int32_t hpRegen = classData.value("regenHpPerTurn", 0);
	int32_t mpRegen = classData.value("regenMpPerTurn", character.GetMpRegenPerTurn());

	if (hpRegen > 0)
	{
		character.Heal(hpRegen);
	}

	if (mpRegen > 0)
	{
		character.RestoreMp(mpRegen);
	}

	// Ensure MP doesn't exceed computed max
	int32_t computedMax = character.GetMaxMp();
	if (character.GetMp() > computedMax)
	{
		character.SetMp(computedMax);
	}
	if (character.GetMaxMp() != computedMax)
	{
		character.SetMaxMp(computedMax);
	}
}
