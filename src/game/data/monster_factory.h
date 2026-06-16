#pragma once

#include <string>
#include "game/combat/monster.h"

struct MonsterFactory final
{
	[[nodiscard]] static Monster Create(const std::string& typeId, int32_t level);
};
