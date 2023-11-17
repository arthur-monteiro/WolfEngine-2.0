#include "OrthographicCamera.h"

Wolf::OrthographicCamera::OrthographicCamera(const glm::vec3& initialCenter, float initialRadius, float initialHeightFromCenter, const glm::vec3& initialDirection)
{
	m_center = initialCenter;
	m_radius = initialRadius;
	m_heightFromCenter = initialHeightFromCenter;
	m_direction = initialDirection;
}

void Wolf::OrthographicCamera::update(const CameraUpdateContext& context)
{
	m_previousViewMatrix = m_viewMatrix;
	m_viewMatrix = glm::lookAt(m_center - m_heightFromCenter * normalize(m_direction), m_center, glm::vec3(0.0f, 1.0f, 0.0f));
	m_projectionMatrix = glm::ortho(-m_radius, m_radius, -m_radius, m_radius, -30.0f * 6.0f, 30.0f * 6.0f);

	updateGraphic(glm::vec2(0.0f));
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
