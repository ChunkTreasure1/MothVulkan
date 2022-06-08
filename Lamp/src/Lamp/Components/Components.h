#pragma once

#include "Lamp/Asset/Asset.h"

#include <Wire/ECS/Serialization.h>
#include <glm/glm.hpp>

namespace Lamp
{
	struct TagComponent
	{
		TagComponent() = default;
		TagComponent(const std::string& aTag)
			: tag(aTag)
		{ }

		std::string tag;
	
		SERIALIZE_COMPONENT(TagComponent, "{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid);
	};

	struct TransformComponent
	{
		TransformComponent() = default;
		TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
			: position(pos), rotation(rot), scale(scale)
		{ }

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		
		SERIALIZE_COMPONENT(TransformComponent, "{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid);
	};

	struct MeshComponent
	{
		MeshComponent() = default;
		MeshComponent(const AssetHandle& asset)
			: handle(asset)
		{ }

		AssetHandle handle = Asset::Null();

		SERIALIZE_COMPONENT(MeshComponent, "{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid)
	};

	REGISTER_COMPONENT(TagComponent);
	REGISTER_COMPONENT(TransformComponent);
	REGISTER_COMPONENT(MeshComponent);
}