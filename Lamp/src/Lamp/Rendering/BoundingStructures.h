#pragma once

#include <glm/glm.hpp>

namespace Lamp
{
	struct BoundingSphere
	{
		glm::vec3 center;
		float radius;
	};
}