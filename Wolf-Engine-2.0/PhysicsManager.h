#pragma once

#include "PhysicShapes.h"
#include "ResourceNonOwner.h"
#include "ResourceUniqueOwner.h"

namespace Wolf
{
	namespace Physics
	{
		class PhysicsManager
		{
		public:
			void addStaticRectangle(const Rectangle& rectangle);

			using DynamicShapeId = uint32_t;
			static constexpr DynamicShapeId INVALID_DYNAMIC_SHAPE_ID = -1;
			[[nodiscard]] DynamicShapeId addDynamicRectangle(const Rectangle& rectangle, void* instance);
			[[nodiscard]] DynamicShapeId addDynamicBox(const Box& box, void* instance);
			void removeDynamicShape(DynamicShapeId shapeId);

			struct RayCastResult
			{
				bool collision;

				NullableResourceNonOwner<Shape> shape;
				glm::vec3 hitPoint;
				DynamicShapeId dynamicShapeId = INVALID_DYNAMIC_SHAPE_ID;
				void* instance = nullptr;
			};
			[[nodiscard]] RayCastResult rayCastAnyHit(const glm::vec3& rayOrigin, const glm::vec3& rayEnd);
			[[nodiscard]] RayCastResult rayCastClosestHit(const glm::vec3& rayOrigin, const glm::vec3& rayEnd);

		private:
			struct StaticShape
			{
				ResourceUniqueOwner<Shape> m_shape;

                explicit StaticShape(ResourceUniqueOwner<Shape> shape) : m_shape(std::move(shape)) {}
			};
			std::vector<StaticShape> m_staticShapes;

			struct DynamicShape
			{
				ResourceUniqueOwner<Shape> m_shape;
				void* m_instance;

                DynamicShape(ResourceUniqueOwner<Shape> shape, void* instance) : m_shape(std::move(shape)), m_instance(instance) {}
			};
			std::vector<DynamicShape> m_dynamicShapes;
			std::vector<uint32_t> m_holesInDynamicShapesArray;
		};
	}
}
