#pragma once

#include <glm/glm.hpp>

namespace Lamp
{
	static bool AbsEqualVector(const glm::vec3& aFirst, const glm::vec3& aSecond)
	{
		return std::abs(aFirst.x - aSecond.x) < std::numeric_limits<float>::epsilon() &&
			std::abs(aFirst.y - aSecond.y) < std::numeric_limits<float>::epsilon() &&
			std::abs(aFirst.z - aSecond.z) < std::numeric_limits<float>::epsilon();
	}

	static bool AbsEqualVector(const glm::vec2& aFirst, const glm::vec2& aSecond)
	{
		return std::abs(aFirst.x - aSecond.x) < std::numeric_limits<float>::epsilon() &&
			std::abs(aFirst.y - aSecond.y) < std::numeric_limits<float>::epsilon();
	}	

	struct Vertex
	{
		Vertex() = default;
		Vertex(const glm::vec3& position)
			: position(position)
		{}
		
		Vertex(const glm::vec3& position, const glm::vec2& texCoords)
			: position(position), textureCoords(texCoords)
		{}

		glm::vec3 position = glm::vec3(0.f);
		glm::vec3 normal = glm::vec3(0.f);
		glm::vec3 tangent = glm::vec3(0.f);
		glm::vec3 bitangent = glm::vec3(0.f);
		glm::vec2 textureCoords = glm::vec2(0.f);

		bool operator==(const Vertex& aVert) const
		{
			bool bPos = AbsEqualVector(position, aVert.position);
			bool bNorm = AbsEqualVector(normal, aVert.normal);
			bool bTex = AbsEqualVector(textureCoords, aVert.textureCoords);

			return bPos && bNorm && bTex;
		}
	};
}