#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Wolf
{
	namespace Physics
	{
		static std::vector<std::string> PHYSICS_SHAPE_STRING_LIST = { "Rectangle" };
		enum class PhysicsShapeType : uint32_t { Rectangle = 0 };

		class Shape
		{
		public:
			virtual PhysicsShapeType getType() const = 0;
			virtual const std::string& getName() const { return m_name; }

			struct ShapeRayCastResult
			{
				bool collision;
				glm::vec3 hitPoint;
			};
			virtual ShapeRayCastResult rayCast(const glm::vec3& rayOrigin, const glm::vec3& rayEnd) = 0;

		protected:
			Shape(const std::string& name) : m_name(name) {}

		private:
			std::string m_name;
		};

		class Rectangle : public Shape
		{
		public:
			Rectangle(const std::string& name, const glm::vec3& p0, const glm::vec3& s1, const glm::vec3& s2) : Shape(name), m_p0(p0), m_s1(s1), m_s2(s2)
			{
			}

			PhysicsShapeType getType() const override { return PhysicsShapeType::Rectangle; }
			const glm::vec3& getP0() const { return m_p0; }
			const glm::vec3& getS1() const { return m_s1; }
			const glm::vec3& getS2() const { return m_s2; }

			ShapeRayCastResult rayCast(const glm::vec3& rayOrigin, const glm::vec3& rayEnd) override
			{
				ShapeRayCastResult result;

				glm::vec3 rectangleNormal = glm::normalize(glm::cross(m_s1, m_s2));

				glm::vec3 rayDirection = glm::normalize(rayEnd - rayOrigin);
				float rayDirectionDotNormal = glm::dot(rayDirection, rectangleNormal);
				if (rayDirectionDotNormal == 0.0f)
				{
					result.collision = false;
					return result;
				}

				float a = (glm::dot(m_p0 - rayOrigin, rectangleNormal)) / rayDirectionDotNormal;
				glm::vec3 p = rayOrigin + a * rayDirection;

				glm::vec3 q1 = m_p0 + m_s1 * glm::dot(m_s1, p - m_p0) / glm::dot(m_s1, m_s1);
				glm::vec3 q2 = m_p0 + m_s2 * glm::dot(m_s2, p - m_p0) / glm::dot(m_s2, m_s2);

				constexpr float DISTANCE_EPSILON = 0.01f;
				if ((glm::distance(p, rayOrigin) < DISTANCE_EPSILON || glm::distance(p, rayEnd) < DISTANCE_EPSILON // point is near ray origin or end
					|| (glm::dot(p - rayOrigin, rayEnd - rayOrigin) > 0.0f && glm::distance(p, rayOrigin) < glm::distance(rayEnd, rayOrigin))) // point is between ray origin and end
					&& (glm::dot(q1 - m_p0, m_s1) > 0.0f && glm::length(q1 - m_p0) < glm::length(m_s1) && glm::dot(q2 - m_p0, m_s2) > 0.0f && glm::length(q2 - m_p0) < glm::length(m_s2))) // point is on the rectangle
				{
					result.collision = true;
					result.hitPoint = p;
					return result;
				}

				result.collision = false;
				return result;
			}

		private:
			glm::vec3 m_p0;
			glm::vec3 m_s1;
			glm::vec3 m_s2;
		};
	}
}
