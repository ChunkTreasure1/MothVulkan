#pragma once

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"

#include <GLFW/glfw3.h>

#include <utility> 

namespace Lamp
{
	class Input
	{
	protected:
		Input() = default;

	public:
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		inline static bool IsKeyPressed(int keyCode) 
		{ 
			auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
			auto state = glfwGetKey(window, keyCode);

			return state == GLFW_PRESS || state == GLFW_REPEAT;
		}
		
		inline static bool IsKeyReleased(int keyCode) 
		{ 
			auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
			auto state = glfwGetKey(window, keyCode);

			return state == GLFW_RELEASE;
		}
		
		inline static bool IsMouseButtonPressed(int button) 
		{ 
			auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
			auto state = glfwGetMouseButton(window, button);

			return state == GLFW_PRESS;
		}

		inline static bool IsMouseButtonReleased(int button) 
		{ 
			auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
			auto state = glfwGetMouseButton(window, button);

			return state == GLFW_RELEASE;
		}

		inline static std::pair<float, float> GetMousePosition() 
		{ 
			auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
			double xPos, yPos;
			glfwGetCursorPos(window, &xPos, &yPos);

			return { (float)xPos, (float)yPos };
		}

		inline static float GetMouseX() 
		{ 
			auto [x, y] = GetMousePosition();
			return x;
		}

		inline static float GetMouseY() 
		{
			auto [x, y] = GetMousePosition();
			return y;
		}

	};
}