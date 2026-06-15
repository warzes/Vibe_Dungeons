#pragma once

class InputManager;
class DeltaTime;
class Renderer;

enum class StateAction
{
	None,
	Push,
	Pop,
	Replace
};

class GameState
{
public:
	GameState() noexcept = default;
	virtual ~GameState() noexcept = default;

	GameState(const GameState&) = delete;
	GameState& operator=(const GameState&) = delete;
	GameState(GameState&&) = delete;
	GameState& operator=(GameState&&) = delete;

	virtual void OnEnter() noexcept {}
	virtual void OnExit() noexcept {}
	virtual void OnPause() noexcept {}
	virtual void OnResume() noexcept {}

	virtual void FixedUpdate(float /*fixedDt*/) noexcept {}
	virtual void HandleEvent(const SDL_Event& event) noexcept = 0;
	virtual void Update(const DeltaTime& dt) noexcept = 0;
	virtual void Render() noexcept = 0;
	virtual void RenderScene(Renderer& /*renderer*/) noexcept {}
	virtual void RenderOverlay(const glm::mat4& /*viewProj*/) noexcept {}

	[[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
};

class GameStateMachine final
{
public:
	using StateFactory = std::function<std::unique_ptr<GameState>()>;

	GameStateMachine() noexcept = default;
	~GameStateMachine() noexcept = default;

	void RegisterState(std::string_view name, StateFactory factory);

	void PushState(std::unique_ptr<GameState> state);
	void PushState(std::string_view name);
	void PopState() noexcept;
	void ReplaceState(std::unique_ptr<GameState> state);
	void ReplaceState(std::string_view name);

	void FixedUpdate(float fixedDt) noexcept;
	void HandleEvent(const SDL_Event& event) noexcept;
	void Update(const DeltaTime& dt) noexcept;
	void Render() noexcept;
	void RenderScene(Renderer& renderer) noexcept;
	void RenderOverlay(const glm::mat4& viewProj) noexcept;

	[[nodiscard]] GameState* CurrentState() noexcept;
	[[nodiscard]] bool IsEmpty() const noexcept
	{
		return m_stack.empty();
	}

	void ProcessActions() noexcept;

private:
	std::vector<std::unique_ptr<GameState>> m_stack;
	struct transparent_hash final
	{
		using is_transparent = void;
		[[nodiscard]] size_t operator()(std::string_view sv) const noexcept
		{
			return std::hash<std::string_view>{}(sv);
		}
		[[nodiscard]] size_t operator()(const std::string& s) const noexcept
		{
			return std::hash<std::string>{}(s);
		}
	};
	std::unordered_map<std::string, StateFactory, transparent_hash, std::equal_to<>> m_factories;

	struct PendingAction
	{
		StateAction action;
		std::unique_ptr<GameState> state;
	};
	std::vector<PendingAction> m_pendingActions;

	void applyPush(std::unique_ptr<GameState> state);
	void applyPop() noexcept;
	void applyReplace(std::unique_ptr<GameState> state);
};
