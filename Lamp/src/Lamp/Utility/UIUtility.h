#pragma once

#include "Lamp/Core/Base.h"

#include <imgui.h>

namespace Lamp
{
	class Texture2D;
	class Image2D;
}

namespace UI
{
	ImTextureID GetTextureID(Ref<Lamp::Texture2D> texture);
	ImTextureID GetTextureID(Ref<Lamp::Image2D> texture);
	ImTextureID GetTextureID(Lamp::Texture2D* texture);
}
