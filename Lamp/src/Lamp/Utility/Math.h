#pragma once

#include <glm/glm.hpp>

namespace Lamp::Math
{
	glm::vec4 NormalizePlane(glm::vec4 p)
	{
		return p / glm::length(glm::vec3(p));
	}
}