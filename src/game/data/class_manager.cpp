#include "stdafx.h"
#include "game/data/class_manager.h"
#include "core/json_data_manager.h"

const json& ClassManager::GetClass(const std::string& classId)
{
	return JsonDataManager::Instance().GetClassData(classId);
}

void ClassManager::ApplyStartingStats(const std::string& classId, int32_t& hp, int32_t& mp,
	int32_t& maxHp, int32_t& maxMp, int32_t& ac, int32_t& atkBonus,
	int32_t& str, int32_t& dex, int32_t& con, int32_t& intel)
{
	const json& data = JsonDataManager::Instance().GetClassData(classId);
	if (data.is_null() || data.empty())
	{
		return;
	}

	maxHp = data.value("baseHp", 20);
	hp = maxHp;
	maxMp = data.value("baseMp", 0);
	mp = maxMp;
	ac = 10 + data.value("acBonus", 0);
	atkBonus = data.value("atkBonus", 1);
	str = data.value("baseStr", 10);
	dex = data.value("baseDex", 10);
	con = data.value("baseCon", 10);
	intel = data.value("baseInt", 10);
}
