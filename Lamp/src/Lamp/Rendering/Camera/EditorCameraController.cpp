#include "lppch.h"
#include "EditorCameraController.h"

#include "Lamp/Rendering/Camera/Camera.h"
#include "Lamp/Input/Input.h"
#include "Lamp/Input/MouseButtonCodes.h"
#include "Lamp/Input/KeyCodes.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	EditorCameraController::EditorCameraController(float fov, float nearPlane, float farPlane)
		: m_fov(fov), m_nearPlane(nearPlane), m_farPlane(farPlane)
	{
		m_camera = CreateRef<Lamp::Camera>(fov, 16.f / 9.f, nearPlane, farPlane);
	}

	EditorCameraController::~EditorCameraController()
	{
	}

	void EditorCameraController::UpdateProjection(uint32_t width, uint32_t height)
	{
		float aspectRatio = (float)width / (float)height;
		m_camera->SetPerspectiveProjection(m_fov, aspectRatio, m_nearPlane, m_farPlane);
	}

	void EditorCameraController::OnEvent(Lamp::Event& e)
	{
		Lamp::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<Lamp::AppUpdateEvent>(LP_BIND_EVENT_FN(EditorCameraController::OnUpdateEvent));
		dispatcher.Dispatch<Lamp::MouseScrolledEvent>(LP_BIND_EVENT_FN(EditorCameraController::OnMouseScrolled));
	}

	bool EditorCameraController::OnUpdateEvent(Lamp::AppUpdateEvent& e)
	{
		const glm::vec2 mousePos = { Lamp::Input::GetMouseX(), Lamp::Input::GetMouseY() };
		const glm::vec2 deltaPos = (mousePos - m_lastMousePosition);

		if (m_lastRightUp)
		{
			m_lastMousePosition = mousePos;
		}

		if (Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_RIGHT))
		{
			m_lastRightUp = false;

			float xOffset = mousePos.x - m_lastMousePosition.x;
			float yOffset = m_lastMousePosition.y - mousePos.y;

			m_lastMousePosition = mousePos;

			xOffset *= m_sensitivity;
			yOffset *= m_sensitivity;

			glm::vec3 rot = m_camera->GetRotation();
			rot.z = 0.f;

			rot.x += xOffset;
			rot.y -= yOffset;

			if (rot.y > 89.f)
			{
				rot.y = 89.f;
			}
			else if (rot.y < -89.f)
			{
				rot.y = -89.f;
			}

			m_camera->SetRotation(rot);

			if (Lamp::Input::IsKeyDown(LP_KEY_W))
			{
				m_position += m_translationSpeed * m_camera->GetForward() * e.GetTimestep();
			}
			if (Lamp::Input::IsKeyDown(LP_KEY_S))
			{
				m_position -= m_translationSpeed * m_camera->GetForward() * e.GetTimestep();
			}
			if (Lamp::Input::IsKeyDown(LP_KEY_A))
			{
				m_position -= m_translationSpeed * m_camera->GetRight() * e.GetTimestep();
			}
			if (Lamp::Input::IsKeyDown(LP_KEY_D))
			{
				m_position += m_translationSpeed * m_camera->GetRight() * e.GetTimestep();
			}
		}

		if (Lamp::Input::IsMouseButtonReleased(LP_MOUSE_BUTTON_RIGHT))
		{
			m_lastRightUp = true;
		}

		if (Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_MIDDLE) && !Lamp::Input::IsKeyDown(LP_KEY_LEFT_ALT))
		{
			m_position += -m_camera->GetRight() * deltaPos.x;
			m_position += m_camera->GetUp() * deltaPos.y;
		}

		m_camera->SetPosition(m_position);

		return false;
	}

	bool EditorCameraController::OnMouseScrolled(Lamp::MouseScrolledEvent& e)
	{
		if (Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_RIGHT))
		{
			m_translationSpeed += e.GetYOffset() * 0.5f;
			m_translationSpeed = std::min(m_translationSpeed, m_maxTranslationSpeed);

			if (m_translationSpeed < 0.f)
			{
				m_translationSpeed = 0.f;
			}
		}

		return false;
	}
}