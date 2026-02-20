#include "FirstPersonCamera.h"

#include <FrutumCull.h>
#include <glm/gtc/matrix_transform.hpp>

#include "AABB.h"
#include "DescriptorSetGenerator.h"
#include "InputHandler.h"

Wolf::FirstPersonCamera::FirstPersonCamera(glm::vec3 initialPosition, glm::vec3 initialTarget, glm::vec3 verticalAxis,
                                           float sensibility, float speed, float aspect)
{
	m_position = initialPosition;
	m_target = initialTarget;
	m_verticalAxis = verticalAxis;
	m_sensibility = sensibility;
	m_speed = speed;
	m_aspect = aspect;

	m_locked = true;
}

void Wolf::FirstPersonCamera::update(const CameraUpdateContext& context)
{
	glm::vec2 pixelJitter(0.0f);
	if (m_enableJittering)
	{
		const uint32_t JITTER_OFFSET_COUNT = static_cast<uint32_t>(std::size(JITTER_OFFSET));

		pixelJitter.x = ((JITTER_OFFSET[context.m_frameIdx % JITTER_OFFSET_COUNT].x - 0.5f) * 2.0f) / static_cast<float>(context.m_swapChainExtent.width);
		pixelJitter.y = ((JITTER_OFFSET[context.m_frameIdx % JITTER_OFFSET_COUNT].y - 0.5f) * 2.0f) / static_cast<float>(context.m_swapChainExtent.height);
	}

	if (m_overrideViewMatrices)
	{
		m_projectionMatrix = m_overridenProjectionMatrix;

		updateGraphic(pixelJitter, context);
		return;
	}

	const auto currentTime = std::chrono::high_resolution_clock::now();
	const long long microsecondOffset = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_startTime).count();
	const float secondOffset = static_cast<float>(microsecondOffset) / 1'000'000.0f;
	m_startTime = currentTime;


#ifndef __ANDROID__
	if (context.m_inputHandler->getPreferredInputType() == InputHandler::InputType::KEYBOARD_MOUSE)
	{
		if (m_oldMousePosX < 0 || m_locked)
		{
			context.m_inputHandler->getMousePosition(m_oldMousePosX, m_oldMousePosY);
		}

		float currentMousePosX, currentMousePosY;
		context.m_inputHandler->getMousePosition(currentMousePosX, currentMousePosY);

		updateOrientation(static_cast<float>(currentMousePosX - m_oldMousePosX), static_cast<float>(currentMousePosY - m_oldMousePosY));
		m_oldMousePosX = currentMousePosX;
		m_oldMousePosY = currentMousePosY;

		if (!m_locked)
		{
			if (context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_W) || context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_Z))
			{
				m_position = m_position + m_orientation * (secondOffset * m_speed);
				m_target = m_position + m_orientation;
			}
			else if (context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_S))
			{
				m_position = m_position - m_orientation * (secondOffset * m_speed);
				m_target = m_position + m_orientation;
			}

			if (context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_A) || context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_Q))
			{
				m_position = m_position + m_lateralDirection * (secondOffset * m_speed);
				m_target = m_position + m_orientation;
			}

			else if (context.m_inputHandler->keyPressedThisFrameOrMaintained(GLFW_KEY_D))
			{
				m_position = m_position - m_lateralDirection * (secondOffset * m_speed);
				m_target = m_position + m_orientation;
			}
		}
	}
#endif
	if (context.m_inputHandler->getPreferredInputType() == InputHandler::InputType::GAMEPAD && !m_locked)
	{
		float rotX, rotY;
		context.m_inputHandler->getJoystickSpeedForGamepad(0, 1, rotX, rotY);

		// Sensitivity factor for rotation speed (degrees/radians per second)
		const float rotationSensitivity = 100.0f;
		if (std::abs(rotX) > 0.1f || std::abs(rotY) > 0.1f) // Basic deadzone check
		{
			updateOrientation(rotX * rotationSensitivity * secondOffset,
							  rotY * rotationSensitivity * secondOffset);
		}

		// 3. Handle Movement (Left Stick - usually index 0)
		float moveX, moveY;
		context.m_inputHandler->getJoystickSpeedForGamepad(0, 0, moveX, moveY);

		if (std::abs(moveY) > 0.1f)
		{
			m_position = m_position - m_orientation * (moveY * secondOffset * m_speed);
		}

		if (std::abs(moveX) > 0.1f)
		{
			m_position = m_position - m_lateralDirection * (moveX * secondOffset * m_speed);
		}

		// 4. Finalize Target
		m_target = m_position + m_orientation;
	}

	m_previousViewMatrix = m_viewMatrix;
	m_viewMatrix = glm::lookAt(m_position, m_target, m_verticalAxis);

	m_projectionMatrix = glm::perspective(m_radFOV, m_aspect, m_near, m_far);
	m_projectionMatrix[1][1] *= -1;

    if (context.m_screenRotationInDegrees != 0.0f)
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(context.m_screenRotationInDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
        m_projectionMatrix = rotation * m_projectionMatrix;
    }

	updateGraphic(pixelJitter, context);
}

const glm::mat4& Wolf::FirstPersonCamera::getViewMatrix() const
{
	return m_viewMatrix;
}

const glm::mat4& Wolf::FirstPersonCamera::getPreviousViewMatrix() const
{
	return m_previousViewMatrix;
}

const glm::mat4& Wolf::FirstPersonCamera::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

glm::vec3 Wolf::FirstPersonCamera::getPosition() const
{
	return m_position;
}

bool Wolf::FirstPersonCamera::isAABBVisible(const AABB& aabb) const
{
	return true;

	const glm::mat4 viewProj = m_projectionMatrix * m_viewMatrix;

	const Frustum frustum(viewProj);
	return frustum.IsBoxVisible(aabb.getMin(), aabb.getMax());
}

void Wolf::FirstPersonCamera::overrideMatrices(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
	m_viewMatrix = viewMatrix;
	m_previousViewMatrix = viewMatrix;
	m_overridenProjectionMatrix = projectionMatrix;

	m_overrideViewMatrices = true;
}

void Wolf::FirstPersonCamera::updateOrientation(float xOffset, float yOffset)
{
	m_phi -= yOffset * m_sensibility;
	m_theta -= xOffset * m_sensibility;

	if (m_phi > glm::half_pi<float>() - OFFSET_ANGLES)
		m_phi = glm::half_pi<float>() - OFFSET_ANGLES;
	else if (m_phi < -glm::half_pi<float>() + OFFSET_ANGLES)
		m_phi = -glm::half_pi<float>() + OFFSET_ANGLES;

	if (m_verticalAxis.x == 1.0f)
	{
		m_orientation.x = glm::sin(m_phi);
		m_orientation.y = glm::cos(m_phi) * glm::cos(m_theta);
		m_orientation.z = glm::cos(m_phi) * glm::sin(m_theta);
	}

	else if (m_verticalAxis.y == 1.0f)
	{
		m_orientation.x = glm::cos(m_phi) * glm::sin(m_theta);
		m_orientation.y = glm::sin(m_phi);
		m_orientation.z = glm::cos(m_phi) * glm::cos(m_theta);
	}

	else
	{
		m_orientation.x = glm::cos(m_phi) * glm::cos(m_theta);
		m_orientation.y = glm::cos(m_phi) * glm::sin(m_theta);
		m_orientation.z = glm::sin(m_phi);
	}

	m_lateralDirection = glm::normalize(glm::cross(m_verticalAxis, m_orientation));
	m_target = m_position + m_orientation;
}