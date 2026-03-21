#include "BoundingSphere.h"

#include "AABB.h"

Wolf::BoundingSphere::BoundingSphere(const glm::vec3& center, float radius) : m_center(center), m_radius(radius)
{
}

Wolf::BoundingSphere::BoundingSphere(const AABB& aabb)
{
    m_center = aabb.getCenter();
    float maxDistanceFromCenter = std::max(glm::distance(aabb.getCenter(), aabb.getMin()), glm::distance(aabb.getCenter(), aabb.getMax()));
    m_radius = maxDistanceFromCenter;
}
