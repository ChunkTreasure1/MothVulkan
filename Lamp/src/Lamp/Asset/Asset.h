#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Core/UUID.h"

#include <unordered_map>
#include <string>
#include <filesystem>
#include <xhash>

namespace Lamp
{
	typedef UUID AssetHandle;

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1),
	};

	enum class AssetType : uint16_t
	{
		None = 0,
		Mesh,
		MeshSource,
		Animation,
		Skeleton,
		Texture,
		Material,
		Shader,
		RenderPipeline,
		RenderPass,
		RenderGraph
	};

	inline static std::unordered_map<std::string, AssetType> s_assetExtensionsMap =
	{
		{ ".fbx", AssetType::MeshSource },
		{ ".gltf", AssetType::MeshSource },
		{ ".glb", AssetType::MeshSource },
		{ ".lgf", AssetType::Mesh },

		{ ".lpsk", AssetType::Skeleton },
		{ ".lpsa", AssetType::Animation },

		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".tga", AssetType::Texture },
		{ ".ktx", AssetType::Texture },
		{ ".dds", AssetType::Texture },
		
		{ ".lpsdef", AssetType::Shader },
		{ ".lprpdef", AssetType::RenderPipeline },
		{ ".lprp", AssetType::RenderPass },
		{ ".lprg", AssetType::RenderGraph },
		{ ".lpmat", AssetType::Material }
	};

	class Asset
	{
	public:
		virtual ~Asset() = default;

		inline bool IsValid() const { return ((flags & (uint16_t)AssetFlag::Missing) | (flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		inline virtual bool operator==(const Asset& other)
		{
			return handle = other.handle;
		}

		inline virtual bool operator!=(const Asset& other)
		{
			return !(*this == other);
		}

		inline bool IsFlagSet(AssetFlag flag) { return (flags & (uint16_t)flag) != 0; }
		inline void SetFlag(AssetFlag flag, bool state)
		{
			if (state)
			{
				flags |= (uint16_t)flag;
			}
			else
			{
				flags &= ~(uint16_t)flag;
			}
		}
		
		inline static const AssetHandle Null() { return AssetHandle(0); }

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetType() = 0;

		uint16_t flags = (uint16_t)AssetFlag::None;
		AssetHandle handle;
		std::filesystem::path path;
	};
}

namespace std
{
	template<>
	struct hash<Lamp::AssetHandle>
	{
		size_t operator()(const Lamp::AssetHandle& handle) const
		{
			return hash<uint64_t>()(handle);
		}
	};
}