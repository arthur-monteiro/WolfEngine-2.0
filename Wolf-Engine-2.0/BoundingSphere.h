#pragma once

#include <glm/glm.hpp>

namespace Wolf
{
	class BoundingSphere
	{
	public:
		BoundingSphere(const glm::vec3& center = glm::vec3(0.0f), float radius = 0.1f);

		BoundingSphere operator* (const glm::vec3& scale) const
		{
			return { m_center, m_radius * glm::max(glm::max(scale.x, scale.y), scale.z) };
		}

		BoundingSphere operator+ (const glm::vec3& translation) const
		{
			return { m_center + translation, m_radius };
		}

		[[nodiscard]] const glm::vec3& getCenter() const { return m_center; }
		[[nodiscard]] float getRadius() const { return m_radius; }

	private:
		glm::vec3 m_center;
		float m_radius;
	};
}
