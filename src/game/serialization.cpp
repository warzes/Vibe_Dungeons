#include "stdafx.h"
#include "game/serialization.h"

void to_json(json& j, const ItemDrop& drop)
{
	j = json{
		{"item", drop.item},
		{"position", drop.position}
	};
}

void from_json(const json& j, ItemDrop& drop)
{
	j.at("item").get_to(drop.item);
	j.at("position").get_to(drop.position);
}
