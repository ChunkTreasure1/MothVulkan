#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace Lamp::Math
{
	inline glm::vec4 NormalizePlane(glm::vec4 p)
	{
		return p / glm::length(glm::vec3(p));
	}

	inline bool DecomposeTransform(const glm::mat4& transform, glm::vec3& trans, glm::vec3& rot, glm::vec3& scale)
	{
		using namespace glm;
		using T = float;

		mat4 localMatrix(transform);
		if (epsilonEqual(localMatrix[3][3], static_cast<float>(0), epsilon<T>()))
		{
			return false;
		}

		if (epsilonNotEqual(localMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(localMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(localMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			localMatrix[0][3] = localMatrix[1][3] = localMatrix[2][3] = static_cast<T>(0);
			localMatrix[3][3] = static_cast<T>(1);
		}

		//Get the transform
		trans = vec3(localMatrix[3]);
		localMatrix[3] = vec4(0, 0, 0, localMatrix[3].w);

		vec3 row[3];
		//Scale and shear
		for (length_t i = 0; i < 3; ++i)
		{
			for (length_t j = 0; j < 3; ++j)
			{
				row[i][j] = localMatrix[i][j];
			}
		}

		scale.x = length(row[0]);
		row[0] = detail::scale(row[0], static_cast<T>(1));
		scale.y = length(row[1]);
		row[1] = detail::scale(row[1], static_cast<T>(1));
		scale.z = length(row[2]);
		row[2] = detail::scale(row[2], static_cast<T>(1));

		rot.y = asin(-row[0][2]);
		if (cos(rot.y) != 0)
		{
			rot.x = atan2(row[1][2], row[2][2]);
			rot.z = atan2(row[0][1], row[0][0]);
		}
		else
		{
			rot.x = atan2(-row[2][0], row[1][1]);
			rot.z = 0;
		}

		return true;
	}
}