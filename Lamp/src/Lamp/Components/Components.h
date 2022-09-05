#pragma once

#include "Lamp/Scripting/ScriptBase.h"
#include "Lamp/Asset/Asset.h"
#include "Lamp/Rendering/RendererStructs.h"

#include <Wire/Serialization.h>
#include <glm/glm.hpp>

#include <array>

namespace Lamp
{
	SERIALIZE_COMPONENT(struct TagComponent
	{
		float t;
		PROPERTY(Name = Tag) std::string tag;

		CREATE_COMPONENT_GUID("{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid);
	}, TagComponent);

	SERIALIZE_COMPONENT(struct TransformComponent
	{
		PROPERTY(Name = Position) glm::vec3 position;
		PROPERTY(Name = Rotation) glm::vec3 rotation;
		PROPERTY(Name = Scale) glm::vec3 scale;

		PROPERTY(Visible = false) bool visible = true;
		PROPERTY(Visible = false) bool locked = false;

		CREATE_COMPONENT_GUID("{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid);
	}, TransformComponent);

	SERIALIZE_COMPONENT(struct MeshComponent
	{
		PROPERTY(Name = Mesh) AssetHandle handle = Asset::Null();

		CREATE_COMPONENT_GUID("{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid)
	}, MeshComponent);

	SERIALIZE_COMPONENT(struct DirectionalLightComponent
	{
		PROPERTY(Name = Color, SpecialType = Color) glm::vec3 color;
		PROPERTY(Name = Intensity) float intensity;

		CREATE_COMPONENT_GUID("{F2E06040-9B60-4A0A-9F13-F8DC4C5A4D47}"_guid);
	}, DirectionalLightComponent);

	SERIALIZE_COMPONENT(struct EnvironmentComponent
	{
		PROPERTY(Name = Environment) AssetHandle environmentHandle = Asset::Null();

		AssetHandle lastEnvironmentHandle = Asset::Null();
		Skybox currentSkybox;

		CREATE_COMPONENT_GUID("{3AFCA974-E750-4BF1-99C8-A7EDF8934C52}"_guid);
	}, EnvironmentComponent);

	SERIALIZE_COMPONENT(struct ScriptComponent
	{
		WireGUID scripts[1];

		CREATE_COMPONENT_GUID("{2CD40BA4-17E3-4D75-AEEB-B8B127FDB2CA}"_guid);
	}, ScriptComponent);
}