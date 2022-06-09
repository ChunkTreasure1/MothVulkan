#pragma once

#include "Lamp/Event/Event.h"
#include "Lamp/Event/ApplicationEvent.h"
#include "Lamp/Event/MouseEvent.h"

namespace Lamp
{
	class Camera;
	class EditorCameraController
	{
	public:
		EditorCameraController(float fov, float nearPlane, float farPlane);
		~EditorCameraController();

		void UpdateProjection(uint32_t width, uint32_t height);
		void OnEvent(Lamp::Event& e);

		inline Ref<Lamp::Camera> GetCamera() { return m_camera; }

	private:
		bool OnUpdateEvent(Lamp::AppUpdateEvent& e);
		bool OnMouseScrolled(Lamp::MouseScrolledEvent& e);

		Ref<Lamp::Camera> m_camera;

		glm::vec2 m_lastMousePosition = { 0.f, 0.f };
		glm::vec3 m_position = { 0.f, 0.f, 0.f };

		float m_fov = 45.f;
		float m_nearPlane = 0.1f;
		float m_farPlane = 100.f;

		float m_translationSpeed = 5.f;
		float m_scrollTranslationSpeed = 30.f;

		float m_maxTranslationSpeed = 40.f;
		float m_sensitivity = 0.12f;

		bool m_lastRightUp = false;
	};
}