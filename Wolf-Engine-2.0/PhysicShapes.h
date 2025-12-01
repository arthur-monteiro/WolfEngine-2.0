#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Wolf
{
	namespace Physics
	{
		static std::vector<std::string> PHYSICS_SHAPE_STRING_LIST = { "Rectangle", "Box" };
		enum class PhysicsShapeType : uint32_t { Rectangle = 0, Box = 1 };

		class Shape
		{
		public:
			virtual ~Shape() = default;

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

		class Box : public Shape
		{
		public:
			Box(const std::string& name, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const glm::vec3& rotation) : Shape(name), m_aabbMin(aabbMin), m_aabbMax(aabbMax), m_rotation(rotation)
			{
			}

			PhysicsShapeType getType() const override { return PhysicsShapeType::Box; }
			const glm::vec3& getAABBMin() const { return m_aabbMin; }
			const glm::vec3& getAABBMax() const { return m_aabbMax; }
			const glm::vec3& getRotation() const { return m_rotation; }

			ShapeRayCastResult rayCast(const glm::vec3& rayOrigin, const glm::vec3& rayEnd) override
			{
				ShapeRayCastResult result;
				result.collision = false;

				// Build box local transform (translate to center, then rotate X->Y->Z)
				glm::vec3 center = (m_aabbMin + m_aabbMax) * 0.5f;
				glm::vec3 halfExtents = (m_aabbMax - m_aabbMin) * 0.5f;

				glm::mat4 transform(1.0f);
				transform = glm::translate(transform, center);
				transform = glm::rotate(transform, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
				transform = glm::rotate(transform, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
				transform = glm::rotate(transform, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

				glm::mat4 invTransform = glm::inverse(transform);

				// Transform ray into box local space
				glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(rayOrigin, 1.0f));
				glm::vec3 localEnd = glm::vec3(invTransform * glm::vec4(rayEnd, 1.0f));
				glm::vec3 d = localEnd - localOrigin;

				const float EPS = 1e-8f;
				float tMin = 0.0f;
				float tMax = 1.0f;

				// Slab method for segment [0,1]
				for (int i = 0; i < 3; ++i)
				{
					if (std::abs(d[i]) < EPS)
					{
						// Ray is parallel to slab; if origin not within slab, no hit
						if (localOrigin[i] < -halfExtents[i] || localOrigin[i] > halfExtents[i])
							return result;
					}
					else
					{
						float invD = 1.0f / d[i];
						float t1 = (-halfExtents[i] - localOrigin[i]) * invD;
						float t2 = (halfExtents[i] - localOrigin[i]) * invD;
						if (t1 > t2) std::swap(t1, t2);

						tMin = std::max(tMin, t1);
						tMax = std::min(tMax, t2);

						if (tMin > tMax)
							return result;
					}
				}

				// Check overlap with segment [0,1]
				if (tMax < 0.0f || tMin > 1.0f)
					return result;

				float tHit = tMin;
				if (tHit < 0.0f) tHit = 0.0f;
				if (tHit > 1.0f) tHit = 1.0f;

				glm::vec3 localHit = localOrigin + tHit * d;
				glm::vec3 worldHit = glm::vec3(transform * glm::vec4(localHit, 1.0f));

				result.collision = true;
				result.hitPoint = worldHit;
				return result;
			}

		private:
			glm::vec3 m_aabbMin;
			glm::vec3 m_aabbMax;
			glm::vec3 m_rotation;
		};
	}
}
