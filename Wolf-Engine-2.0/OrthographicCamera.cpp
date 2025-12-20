#include "OrthographicCamera.h"

Wolf::OrthographicCamera::OrthographicCamera(const glm::vec3& initialCenter, float initialRadius, float initialHeightFromCenter, const glm::vec3& initialDirection, float near, float far)
{
	m_center = initialCenter;
	m_radius = initialRadius;
	m_heightFromCenter = initialHeightFromCenter;
	m_direction = initialDirection;
	m_near = near;
	m_far = far;
}

void Wolf::OrthographicCamera::update(const CameraUpdateContext& context)
{
	m_previousViewMatrix = m_viewMatrix;

	glm::vec3 eye = m_center - m_heightFromCenter * glm::normalize(m_direction);
	glm::vec3 forward = glm::normalize(m_center - eye);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	if (glm::abs(glm::dot(forward, up)) > 0.999f)
	{
		// if looking straight up or down, use the Z-axis as a temporary Up
		up = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	m_viewMatrix = glm::lookAt(eye, m_center, up);

	m_projectionMatrix = glm::ortho(-m_radius, m_radius, -m_radius, m_radius, m_near, m_far);
	m_projectionMatrix[1][1] *= -1;

	updateGraphic(glm::vec2(0.0f), context);
}

const glm::mat4& Wolf::OrthographicCamera::getViewMatrix() const
{
	return m_viewMatrix;
}

const glm::mat4& Wolf::OrthographicCamera::getPreviousViewMatrix() const
{
	return m_previousViewMatrix;
}

const glm::mat4& Wolf::OrthographicCamera::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

glm::vec3 Wolf::OrthographicCamera::getPosition() const
{
	return m_center - m_heightFromCenter * normalize(m_direction);
}

bool Wolf::OrthographicCamera::isAABBVisible(const AABB& aabb) const
{
	// TODO
	return true;
}
