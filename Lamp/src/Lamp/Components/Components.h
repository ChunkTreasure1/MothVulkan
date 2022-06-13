#pragma once

#include "Lamp/Asset/Asset.h"

#include <Wire/ECS/Serialization.h>
#include <glm/glm.hpp>

namespace Lamp
{
	REGISTER_COMPONENT(struct TagComponent
	{
		std::string tag;

		SERIALIZE_COMPONENT(TagComponent, "{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid);
	}, TagComponent);

	REGISTER_COMPONENT(struct TransformComponent
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		SERIALIZE_COMPONENT(TransformComponent, "{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid);
	}, TransformComponent);

	REGISTER_COMPONENT(struct MeshComponent
	{
		AssetHandle handle = Asset::Null();

		SERIALIZE_COMPONENT(MeshComponent, "{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid)
	}, MeshComponent);
}