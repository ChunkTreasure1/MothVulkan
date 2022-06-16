#pragma once

#include "Lamp/Asset/Asset.h"

#include <Wire/ECS/Serialization.h>
#include <glm/glm.hpp>

namespace Lamp
{
	SERIALIZE_COMPONENT(struct TagComponent
	{
		std::string tag;

		CREATE_COMPONENT_GUID("{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid);
	}, TagComponent);

	SERIALIZE_COMPONENT(struct TransformComponent
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		CREATE_COMPONENT_GUID("{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid);
	}, TransformComponent);

	SERIALIZE_COMPONENT(struct MeshComponent
	{
		AssetHandle handle = Asset::Null();

		CREATE_COMPONENT_GUID("{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid)
	}, MeshComponent);

	SERIALIZE_COMPONENT(struct DirectionalLightComponent
	{
		glm::vec3 color;
		float intensity;

		CREATE_COMPONENT_GUID("{F2E06040-9B60-4A0A-9F13-F8DC4C5A4D47}"_guid);
	}, DirectionalLightComponent);
}