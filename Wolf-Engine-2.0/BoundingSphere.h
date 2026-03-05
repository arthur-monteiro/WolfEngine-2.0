#pragma once

#include <glm/glm.hpp>

namespace Wolf
{
	class BoundingSphere
	{
	public:
		BoundingSphere(const glm::vec3& center = glm::vec3(0.0f), float radius = 0.1f);

		BoundingSphere operator* (const glm::mat4& transform) const
		{
			glm::vec4 boundingSphereCenter = transform * glm::vec4(m_center, 1.0);
			float boundingSphereRadius = m_radius;
			float sx = glm::dot(glm::vec3(transform[0]), glm::vec3(transform[0]));
			float sy = glm::dot(glm::vec3(transform[1]), glm::vec3(transform[1]));
			float sz = glm::dot(glm::vec3(transform[2]), glm::vec3(transform[2]));
			float maxScale = glm::sqrt(glm::max(sx, glm::max(sy, sz)));
			boundingSphereRadius *= maxScale;

			return { boundingSphereCenter, boundingSphereRadius };
		}

		[[nodiscard]] const glm::vec3& getCenter() const { return m_center; }
		[[nodiscard]] float getRadius() const { return m_radius; }

	private:
		glm::vec3 m_center;
		float m_radius;
	};
}
