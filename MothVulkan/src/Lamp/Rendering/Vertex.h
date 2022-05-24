#pragma once

#include <glm/glm.hpp>

namespace Lamp
{
	struct Vertex
	{
		Vertex() = default;
		Vertex(const glm::vec3& position, const glm::vec2& texCoords)
			: position(position), textureCoords(texCoords)
		{}

		Vertex(const glm::vec3& position)
			: position(position)
		{}

		glm::vec3 position = glm::vec3(0.f);
		glm::vec3 normal = glm::vec3(0.f);
		glm::vec3 tangent = glm::vec3(0.f);
		glm::vec3 bitangent = glm::vec3(0.f);
		glm::vec2 textureCoords = glm::vec2(0.f);
	};
}