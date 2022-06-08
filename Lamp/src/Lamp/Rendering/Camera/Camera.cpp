#include "lppch.h"
#include "Camera.h"

namespace Lamp
{
	Camera::Camera(float fov, float aspect, float nearPlane, float farPlane)
		: m_fieldOfView(fov), m_aspectRatio(aspect), m_nearPlane(nearPlane), m_farPlane(farPlane)
	{
		m_projectionMatrix = glm::perspective(glm::radians(m_fieldOfView), aspect, m_nearPlane, m_farPlane);
		m_viewMatrix = glm::mat4(1.f);
	}
	
	Camera::Camera(float left, float right, float bottom, float top, float nearPlane, float farPlane)
		: m_nearPlane(nearPlane), m_farPlane(farPlane)
	{
		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.f, 1.f);
		m_viewMatrix = glm::mat4(1.f);
	}

	void Camera::SetPerspectiveProjection(float fov, float aspect, float nearPlane, float farPlane)
	{
		m_fieldOfView = fov;
		m_aspectRatio = aspect;
		m_nearPlane = nearPlane;
		m_farPlane = farPlane;

		m_projectionMatrix = glm::perspective(glm::radians(m_fieldOfView), m_aspectRatio, m_nearPlane, m_farPlane);
	}

	void Camera::SetOrthographicProjection(float left, float right, float bottom, float top)
	{
		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.f, 1.f);
	}

	glm::vec3 Camera::GetUp() const
	{
		return glm::rotate(GetOrientation(), glm::vec3{ 0.f, 1.f, 0.f });
	}

	glm::vec3 Camera::GetRight() const
	{
		return glm::rotate(GetOrientation(), glm::vec3{ 1.f, 0.f, 0.f });
	}

	glm::vec3 Camera::GetForward() const
	{
		return glm::rotate(GetOrientation(), glm::vec3{ 0.f, 0.f, -1.f });
	}

	glm::quat Camera::GetOrientation() const
	{
		return glm::quat(glm::vec3(glm::radians(-m_rotation.y), glm::radians(-m_rotation.x), 0.f));
	}
}