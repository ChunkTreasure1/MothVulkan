#pragma once

#include "Lamp/Core/Base.h"

#include <glm/glm.hpp>

namespace Lamp
{
	class Camera
	{
	public:
		Camera(float fov, float aspect, float nearPlane, float farPlane);
		Camera(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		~Camera() = default;

		void SetPerspectiveProjection(float fov, float aspect, float nearPlane, float farPlane);
		void SetOrthographicProjection(float left, float right, float bottom, float top);

		inline void SetPosition(const glm::vec3& pos) { m_position = pos; RecalculateViewMatrix(); }
		inline void SetRotation(const glm::vec3& rot) { m_rotation = rot; RecalculateViewMatrix(); }

		inline void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
		inline void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
		
		inline const glm::mat4& GetProjection() const { return m_projectionMatrix; }
		inline const glm::mat4& GetView() const { return m_viewMatrix; }

		inline const glm::vec3& GetPosition() const { return m_position; }
		inline const glm::vec3& GetRotation() const { return m_rotation; }

		inline const float GetFieldOfView() const { return m_fieldOfView; }
		inline const float GetAspectRatio() const { return m_aspectRatio; }
		
		inline const float GetNearPlane() const { return m_nearPlane; }
		inline const float GetFarPlane() const { return m_farPlane; }

		glm::vec3 GetUp() const;
		glm::vec3 GetRight() const;
		glm::vec3 GetForward() const;

		glm::quat GetOrientation() const;

	private:
		void RecalculateViewMatrix();

		glm::vec3 m_position = { 0.f, 0.f, 0.f };
		glm::vec3 m_rotation = { 0.f, 0.f, 0.f };

		glm::mat4 m_projectionMatrix = glm::mat4(1.f);
		glm::mat4 m_viewMatrix = glm::mat4(1.f);

		float m_nearPlane = 0.f;
		float m_farPlane = 0.f;
		float m_fieldOfView = 0.f;
		float m_aspectRatio = 0.f;
	};
}