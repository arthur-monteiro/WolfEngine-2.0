#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "CameraInterface.h"
#include "GraphicCameraInterface.h"

namespace Wolf
{
	class OrthographicCamera : public CameraInterface, public GraphicCameraInterface
	{
	public:
		OrthographicCamera(const glm::vec3& initialCenter, float initialRadius, float initialHeightFromCenter, const glm::vec3& initialDirection);

		void update(const CameraUpdateContext& context) override;

		const glm::mat4& getViewMatrix() const override;
		const glm::mat4& getPreviousViewMatrix() const override;
		const glm::mat4& getProjectionMatrix() const override;
		glm::vec3 getPosition() const override;
		float getNear() const override { return -m_heightFromCenter; }
		float getFar() const override { return m_heightFromCenter; }
		glm::vec3 getOrientation() const override { return {0.0f, -1.0f, 0.0f}; }
		float getFOV() const override { return glm::pi<float>(); }

		const DescriptorSet* getDescriptorSet() const override { return GraphicCameraInterface::getDescriptorSet(); }

		void setCenter(const glm::vec3& center) { m_center = center; }
		void setRadius(float radius) { m_radius = radius; }
		void setDirection(const glm::vec3& direction) { m_direction = direction; }

	private:
		glm::vec3 m_center;
		float m_radius;
		float m_heightFromCenter;
		glm::vec3 m_direction;

		glm::mat4 m_viewMatrix;
		glm::mat4 m_previousViewMatrix;

		glm::mat4 m_projectionMatrix;
	};
}
