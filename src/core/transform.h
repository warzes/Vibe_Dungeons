#pragma once

class Transform final
{
public:
	Transform() noexcept = default;

	void SetPosition(const glm::vec3& pos) noexcept
	{
		position = pos;
		m_dirty = true;
	}

	void SetRotation(const glm::vec3& rot) noexcept
	{
		rotation = rot;
		m_dirty = true;
	}

	void SetScale(const glm::vec3& s) noexcept
	{
		scale = s;
		m_dirty = true;
	}

	[[nodiscard]] const glm::vec3& GetPosition() const noexcept
	{
		return position;
	}

	[[nodiscard]] const glm::vec3& GetRotation() const noexcept
	{
		return rotation;
	}

	[[nodiscard]] const glm::vec3& GetScale() const noexcept
	{
		return scale;
	}

	[[nodiscard]] const glm::mat4& LocalMatrix() const noexcept
	{
		if (m_dirty)
		{
			m_cachedLocal = glm::translate(glm::mat4(1.0f), position);
			m_cachedLocal = glm::rotate(m_cachedLocal, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			m_cachedLocal = glm::rotate(m_cachedLocal, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			m_cachedLocal = glm::rotate(m_cachedLocal, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			m_cachedLocal = glm::scale(m_cachedLocal, scale);
			m_dirty = false;
		}
		return m_cachedLocal;
	}

	[[nodiscard]] glm::mat4 WorldMatrix() const noexcept
	{
		glm::mat4 local = LocalMatrix();
		if (m_parent != nullptr)
		{
			return m_parent->WorldMatrix() * local;
		}
		return local;
	}

	void SetParent(Transform* parent) noexcept
	{
		if (m_parent == parent)
		{
			return;
		}
		if (parent != nullptr && wouldCreateCycle(this, parent))
		{
			return;
		}
		// Remove from old parent's child list
		if (m_parent != nullptr)
		{
			auto& siblings = m_parent->m_children;
			auto it = std::ranges::find(siblings, this);
			if (it != siblings.end())
			{
				siblings.erase(it);
			}
		}
		m_parent = parent;
		// Add to new parent's child list
		if (m_parent != nullptr)
		{
			m_parent->m_children.push_back(this);
		}
	}

	[[nodiscard]] Transform* GetParent() const noexcept
	{
		return m_parent;
	}

	[[nodiscard]] const std::vector<Transform*>& GetChildren() const noexcept
	{
		return m_children;
	}

	[[nodiscard]] static bool wouldCreateCycle(const Transform* node, const Transform* newParent) noexcept
	{
		for (const Transform* p = newParent; p != nullptr; p = p->m_parent)
		{
			if (p == node)
			{
				return true;
			}
		}
		return false;
	}

private:
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f); // euler degrees
	glm::vec3 scale = glm::vec3(1.0f);
	Transform* m_parent = nullptr;
	std::vector<Transform*> m_children;

	mutable glm::mat4 m_cachedLocal = glm::mat4(1.0f);
	mutable bool m_dirty = true;
};