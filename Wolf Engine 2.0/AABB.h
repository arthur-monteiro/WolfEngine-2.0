#pragma once

#include <algorithm>
#include <glm/glm.hpp>

namespace Wolf
{
	class AABB
	{
	public:
		AABB(const glm::vec3& min = glm::vec3(0.0f), const glm::vec3& max = glm::vec3(0.0f)) : m_min(min), m_max(max)
		{
			for (uint8_t componentIdx = 0; componentIdx < 3; ++componentIdx)
			{
				if (m_min[componentIdx] > m_max[componentIdx])
				{
					const float cache = m_min[componentIdx];
					m_min[componentIdx] = m_max[componentIdx];
					m_max[componentIdx] = cache;
				}
			}

			static constexpr float MIN_SIZE = 0.01f;
			static constexpr float HALF_MIN_SIZE = MIN_SIZE * 0.5f;

			glm::vec3 size = getSize();
			for (uint8_t componentIdx = 0; componentIdx < 3; ++componentIdx)
			{
				if (size[componentIdx] < MIN_SIZE)
				{
					m_min[componentIdx] -= HALF_MIN_SIZE;
					m_max[componentIdx] += HALF_MIN_SIZE;
				}
			}			
		}

		AABB operator* (const glm::mat4& transform) const
		{
			return { transform * glm::vec4(m_min, 1.0f), transform * glm::vec4(m_max, 1.0f) };
		}

		[[nodiscard]] bool isPointInside(const glm::vec3& point) const
		{
			return point.x > m_min.x && point.y > m_min.y && point.z > m_min.z &&
				point.x < m_max.x && point.y < m_max.y && point.z < m_max.z;
		}

		static constexpr float NO_INTERSECTION = -1.0f;
		[[nodiscard]] float intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const
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

		glm::vec3 getSize() const { return m_max - m_min; }
		glm::vec3 getCenter() const { return (m_min + m_max) / 2.0f; }

		const glm::vec3& getMin() const { return m_min; }
		const glm::vec3& getMax() const { return m_max; }

		glm::vec3 getP0() const { return m_min; }
		glm::vec3 getP1() const { return { m_max.x, m_min.y, m_min.z }; }
		glm::vec3 getP2() const { return { m_min.x, m_max.y, m_min.z }; }
		glm::vec3 getP3() const { return { m_min.x, m_min.y, m_max.z }; }
		glm::vec3 getP4() const { return { m_max.x, m_max.y, m_min.z }; }
		glm::vec3 getP5() const { return { m_max.x, m_min.y, m_max.z }; }
		glm::vec3 getP6() const { return { m_min.x, m_max.y, m_max.z }; }
		glm::vec3 getP7() const { return m_max; }

	private:
		glm::vec3 m_min;
		glm::vec3 m_max;
	};
}
