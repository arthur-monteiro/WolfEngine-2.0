#pragma once

#include <glm/glm.hpp>

namespace Wolf
{
	class AABB
	{
	public:
		AABB(const glm::vec3& min = glm::vec3(0.0f), const glm::vec3& max = glm::vec3(0.0f)) : m_min(min), m_max(max)
		{}

		AABB operator* (const glm::mat4& transform) const
		{
			return { transform * glm::vec4(m_min, 1.0f), transform * glm::vec4(m_max, 1.0f) };
		}

		bool isPointInside(const glm::vec3& point) const
		{
			return point.x > m_min.x && point.y > m_min.y && point.z > m_min.z && 
				point.x < m_max.x && point.y < m_max.y && point.z < m_max.z;
		}

		static constexpr float NO_INTERSECTION = -1.0f;
		float intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const
		{
			const glm::vec3 tMin = (m_min - rayOrigin) / rayDirection;
			const glm::vec3 tMax = (m_max - rayOrigin) / rayDirection;
			const glm::vec3 t1 = min(tMin, tMax);
			const glm::vec3 t2 = max(tMin, tMax);
			const float tNear = std::max(std::max(t1.x, t1.y), t1.z);
			const float tFar = std::min(std::min(t2.x, t2.y), t2.z);

			if (tFar < tNear)
				return NO_INTERSECTION;

			return std::max(tNear, 0.0f);
		}

	private:
		glm::vec3 m_min;
		glm::vec3 m_max;
	};
}