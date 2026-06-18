#pragma once

enum class ActionType : uint8_t
{
	MoveForward,
	MoveBackward,
	TurnLeft,
	TurnRight,
	StrafeLeft,
	StrafeRight,
	MeleeAttack,
	RangedAttack,
	CastSpell,
	UseItem,
	Search,
	Wait
};
