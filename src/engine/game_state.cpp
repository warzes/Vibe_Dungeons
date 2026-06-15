#include "stdafx.h"
#include "engine/game_state.h"
#include "core/logger.h"

void GameStateMachine::RegisterState(std::string_view name, StateFactory factory)
{
	m_factories[std::string(name)] = std::move(factory);
}

void GameStateMachine::PushState(std::unique_ptr<GameState> state)
{
	m_pendingActions.push_back({StateAction::Push, std::move(state)});
}

void GameStateMachine::PushState(std::string_view name)
{
	auto it = m_factories.find(name);
	if (it != m_factories.end())
	{
		m_pendingActions.push_back({StateAction::Push, it->second()});
	}
	else
	{
		Logger::Error(std::string("No factory registered for state: ") + name.data());
	}
}

void GameStateMachine::PopState() noexcept
{
	m_pendingActions.push_back({StateAction::Pop, nullptr});
}

void GameStateMachine::ReplaceState(std::unique_ptr<GameState> state)
{
	m_pendingActions.push_back({StateAction::Replace, std::move(state)});
}

void GameStateMachine::ReplaceState(std::string_view name)
{
	auto it = m_factories.find(name);
	if (it != m_factories.end())
	{
		m_pendingActions.push_back({StateAction::Replace, it->second()});
	}
	else
	{
		Logger::Error(std::string("No factory registered for state: ") + name.data());
	}
}

void GameStateMachine::FixedUpdate(float fixedDt) noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->FixedUpdate(fixedDt);
	}
}

void GameStateMachine::HandleEvent(const SDL_Event& event) noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->HandleEvent(event);
	}
}

void GameStateMachine::Update(const DeltaTime& dt) noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->Update(dt);
	}
}

void GameStateMachine::Render() noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->Render();
	}
}

void GameStateMachine::RenderScene(Renderer& renderer) noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->RenderScene(renderer);
	}
}

void GameStateMachine::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	if (!m_stack.empty())
	{
		m_stack.back()->RenderOverlay(viewProj);
	}
}

GameState* GameStateMachine::CurrentState() noexcept
{
	return m_stack.empty() ? nullptr : m_stack.back().get();
}

void GameStateMachine::ProcessActions() noexcept
{
	for (auto& pending : m_pendingActions)
	{
		switch (pending.action)
		{
			case StateAction::Push:
				applyPush(std::move(pending.state));
				break;
			case StateAction::Pop:
				applyPop();
				break;
			case StateAction::Replace:
				applyReplace(std::move(pending.state));
				break;
			case StateAction::None:
				break;
		}
	}
	m_pendingActions.clear();
}

void GameStateMachine::applyPush(std::unique_ptr<GameState> state)
{
	if (!m_stack.empty())
	{
		m_stack.back()->OnPause();
	}
	m_stack.push_back(std::move(state));
	try
	{
		m_stack.back()->OnEnter();
	}
	catch (...)
	{
		Logger::Error(std::string("State OnEnter threw: ") + m_stack.back()->GetName().data());
		m_stack.pop_back();
		return;
	}
	Logger::Info(std::string("State push: ") + m_stack.back()->GetName().data());
}

void GameStateMachine::applyPop() noexcept
{
	if (m_stack.empty())
	{
		return;
	}
		m_stack.back()->OnExit();
		Logger::Info(std::string("State pop: ") + m_stack.back()->GetName().data());
		m_stack.pop_back();
		if (!m_stack.empty())
		{
			m_stack.back()->OnResume();
		}
}

void GameStateMachine::applyReplace(std::unique_ptr<GameState> state)
{
	if (!m_stack.empty())
	{
		m_stack.back()->OnPause();
		m_stack.back()->OnExit();
		Logger::Info(std::string("State replace: ") + m_stack.back()->GetName().data());
		m_stack.pop_back();
	}
	m_stack.push_back(std::move(state));
	try
	{
		m_stack.back()->OnEnter();
	}
	catch (...)
	{
		Logger::Error(std::string("State OnEnter threw: ") + m_stack.back()->GetName().data());
		m_stack.pop_back();
		return;
	}
	Logger::Info(std::string("State replace with: ") + m_stack.back()->GetName().data());
}
