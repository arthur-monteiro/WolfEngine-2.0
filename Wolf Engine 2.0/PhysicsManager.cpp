#include "PhysicsManager.h"

void Wolf::Physics::PhysicsManager::addStaticRectangle(const Rectangle& rectangle)
{
	m_staticShapes.emplace_back(ResourceUniqueOwner<Shape>(new Rectangle(rectangle)));
}

Wolf::Physics::PhysicsManager::DynamicShapeId Wolf::Physics::PhysicsManager::addDynamicRectangle(const Rectangle& rectangle, void* instance)
{
	if (!m_holesInDynamicShapesArray.empty())
	{
		uint32_t shapeId = m_holesInDynamicShapesArray.back();
		m_holesInDynamicShapesArray.pop_back();
		m_dynamicShapes[shapeId].shape.reset(new Rectangle(rectangle));
		m_dynamicShapes[shapeId].instance = instance;
		return shapeId;
	}

	m_dynamicShapes.emplace_back(ResourceUniqueOwner<Shape>(new Rectangle(rectangle)), instance);
	return static_cast<uint32_t>(m_dynamicShapes.size()) - 1;
}

void Wolf::Physics::PhysicsManager::removeDynamicShape(DynamicShapeId shapeId)
{
	m_dynamicShapes[shapeId].shape.reset(nullptr);
	m_holesInDynamicShapesArray.push_back(shapeId);
}

Wolf::Physics::PhysicsManager::RayCastResult Wolf::Physics::PhysicsManager::rayCastAnyHit(const glm::vec3& rayOrigin, const glm::vec3& rayEnd)
{
	for (StaticShape& staticShape : m_staticShapes)
	{
		Shape::ShapeRayCastResult shapeRayCastResult = staticShape.shape->rayCast(rayOrigin, rayEnd);
		if (shapeRayCastResult.collision)
		{
			RayCastResult rayCastResult;
			rayCastResult.collision = true;
			rayCastResult.shape = staticShape.shape.createNonOwnerResource();
			rayCastResult.hitPoint = shapeRayCastResult.hitPoint;
			return rayCastResult;
		}
	}

	for (uint32_t i = 0; i < m_dynamicShapes.size(); ++i)
	{
		DynamicShape& dynamicShape = m_dynamicShapes[i];

		Shape::ShapeRayCastResult shapeRayCastResult = dynamicShape.shape->rayCast(rayOrigin, rayEnd);
		if (shapeRayCastResult.collision)
		{
			RayCastResult rayCastResult;
			rayCastResult.collision = true;
			rayCastResult.shape = dynamicShape.shape.createNonOwnerResource();
			rayCastResult.hitPoint = shapeRayCastResult.hitPoint;
			rayCastResult.dynamicShapeId = i;
			rayCastResult.instance = dynamicShape.instance;
			return rayCastResult;
		}
	}

	RayCastResult rayCastResult;
	rayCastResult.collision = false;
	return rayCastResult;
}
