#pragma once

class EventBase
{
public:
	virtual ~EventBase() noexcept = default;
};

template<typename T>
concept EventType = std::is_base_of_v<EventBase, T>;

using SubscriptionToken = size_t;

class EventBus final
{
public:
	template<EventType T>
	SubscriptionToken Subscribe(std::function<void(const T&)> callback)
	{
		const auto token = m_nextToken++;
		m_listeners[std::type_index(typeid(T))].push_back(
			{
				token,
				[cb = std::move(callback)](const EventBase& event)
				{
					cb(static_cast<const T&>(event));
				}
			}
		);
		return token;
	}

	template<EventType T>
	void Unsubscribe(SubscriptionToken token)
	{
		auto it = m_listeners.find(std::type_index(typeid(T)));
		if (it != m_listeners.end())
		{
			auto& vec = it->second;
			std::erase_if(vec, [token](const Listener& l) { return l.token == token; });
		}
	}

	template<EventType T>
	void Publish(const T& event)
	{
		auto it = m_listeners.find(std::type_index(typeid(T)));
		if (it != m_listeners.end())
		{
			auto listenersCopy = it->second;
			for (const auto& listener : listenersCopy)
			{
				listener.callback(event);
			}
		}
	}

	void Clear() noexcept
	{
		m_listeners.clear();
	}

private:
	struct Listener
	{
		SubscriptionToken token;
		std::function<void(const EventBase&)> callback;
	};

	SubscriptionToken m_nextToken = 1;

	std::unordered_map<
		std::type_index,
		std::vector<Listener>
	> m_listeners;
};
