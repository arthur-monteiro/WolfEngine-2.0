#pragma once

#include <chrono>

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
	class FirstPersonCamera : public CameraInterface, public GraphicCameraInterface
	{
	public:
		FirstPersonCamera(glm::vec3 initialPosition, glm::vec3 initialTarget, glm::vec3 verticalAxis, float sensibility, float speed, float aspect);

		void update(const CameraUpdateContext& context) override;

		const glm::mat4& getViewMatrix() const override;
		const glm::mat4& getPreviousViewMatrix() const override;
		const glm::mat4& getProjectionMatrix() const override;
		glm::vec3 getPosition() const override;
		float getNear() const override { return m_near; }
		float getFar() const override { return m_far; }
		glm::vec3 getOrientation() const override { return m_orientation; }
		glm::vec3 getTarget() const { return m_target; }
		float getFOV() const override { return m_radFOV; }

		const DescriptorSet* getDescriptorSet() const override { return GraphicCameraInterface::getDescriptorSet(); }

		void setLocked(bool value) { m_locked = value; }
		void setAspect(float aspect) { m_aspect = aspect; }

		void overrideMatrices(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	private:
		void updateOrientation(float xOffset, float yOffset);

		float m_phi = 0.0f;
		float m_theta = 0.0f;
		glm::vec3 m_orientation = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 m_lateralDirection = glm::vec3(-1.0f, 0.0f, 0.0f);

		glm::vec3 m_position = glm::vec3(0.0f);
		glm::vec3 m_target = glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 m_verticalAxis = glm::vec3(0.0f, 1.0f, 0.0f);
		float m_sensibility = 0.5f;
		float m_speed = 0.5f;

		std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();
		float m_oldMousePosX = -1;
		float m_oldMousePosY = -1;

		const float OFFSET_ANGLES = 0.01f;
		bool m_locked = false;

		float m_aspect;
		float m_near = 0.1f;
		float m_far = 50.0f;
		float m_radFOV = glm::radians(45.0f);

		glm::mat4 m_viewMatrix;
		glm::mat4 m_previousViewMatrix;

		glm::mat4 m_projectionMatrix;

		bool m_overrideViewMatrices = false;
		glm::mat4 m_overridenProjectionMatrix;
	};
}
