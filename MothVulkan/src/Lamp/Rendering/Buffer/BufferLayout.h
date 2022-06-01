#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace Lamp
{
	enum class ElementType
	{
		Bool,
		Int,

		UInt,
		UInt2,
		UInt3,
		UInt4,
		
		Float,
		Float2,
		Float3,
		Float4,
	
		Mat3,
		Mat4
	};

	static uint32_t GetSizeFromType(ElementType elementType)
	{
		switch (elementType)
		{
			case ElementType::Bool: return 1;
			case ElementType::Int: return 4;
			case ElementType::UInt: return 4;
			case ElementType::UInt2: return 4 * 4;
			case ElementType::UInt3: return 4 * 4;
			case ElementType::UInt4: return 4 * 4;
			case ElementType::Float: return 4;
			case ElementType::Float2: return 4 * 4;
			case ElementType::Float3: return 4 * 4;
			case ElementType::Float4: return 4 * 4;
			case ElementType::Mat3: return 4 * 3 * 3;
			case ElementType::Mat4: return 4 * 4 * 4;
		}

		return 0;
	}

	static ElementType GetTypeFromString(const std::string& string)
	{
		if (string == "Bool")
		{
			return ElementType::Bool;
		}
		else if (string == "Int")
		{
			return ElementType::Int;
		}
		else if (string == "UInt")
		{
			return ElementType::UInt;
		}
		else if (string == "UInt2")
		{
			return ElementType::UInt2;
		}
		else if (string == "UInt3")
		{
			return ElementType::UInt3;
		}
		else if (string == "UInt4")
		{
			return ElementType::UInt4;
		}
		else if (string == "Float")
		{
			return ElementType::Float;
		}
		else if (string == "Float2")
		{
			return ElementType::Float2;
		}
		else if (string == "Float3")
		{
			return ElementType::Float3;
		}
		else if (string == "Float4")
		{
			return ElementType::Float4;
		}
		else if (string == "Mat3")
		{
			return ElementType::Mat3;
		}
		else if (string == "Mat4")
		{
			return ElementType::Mat4;
		}

		return ElementType::Float;
	}

	static VkFormat LampToVulkanFormat(ElementType type)
	{
		switch (type)
		{
			case ElementType::Bool: return VK_FORMAT_R8_UINT;
			case ElementType::Int: return VK_FORMAT_R8_SINT;
			case ElementType::UInt: return VK_FORMAT_R32_UINT;
			case ElementType::UInt2: return VK_FORMAT_R32G32_UINT;
			case ElementType::UInt3: return VK_FORMAT_R32G32B32_UINT;
			case ElementType::UInt4: return VK_FORMAT_R32G32B32A32_UINT;
			case ElementType::Float: return VK_FORMAT_R32_SFLOAT;
			case ElementType::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case ElementType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ElementType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ElementType::Mat3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ElementType::Mat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		return VK_FORMAT_R8G8B8A8_UNORM;
	}

	struct BufferElement
	{
		BufferElement(ElementType aElementType, const std::string& aName)
			: name(aName), type(aElementType), size(GetSizeFromType(aElementType))
		{ }

		uint32_t GetComponentCount(ElementType elementType)
		{
			switch (elementType)
			{
				case ElementType::Bool: return 1;
				case ElementType::Int: return 1;
				case ElementType::UInt: return 1;
				case ElementType::UInt2: return 2;
				case ElementType::UInt3: return 3;
				case ElementType::UInt4: return 4;
				case ElementType::Float: return 1;
				case ElementType::Float2: return 2;
				case ElementType::Float3: return 3;
				case ElementType::Float4: return 4;
				case ElementType::Mat3: return 3 * 3;
				case ElementType::Mat4: return 4 * 4;
			}

			return 0;
		}

		std::string name;
		uint32_t size;
		uint32_t offset;
		ElementType type;
	};

	class BufferLayout
	{
	public:
		BufferLayout() {}

		BufferLayout(std::initializer_list<BufferElement> elements)
			: m_elements(elements)
		{
			CalculateOffsetAndStride();
		}

		BufferLayout(std::vector<BufferElement> elements)
			: m_elements(elements)
		{
			CalculateOffsetAndStride();
		}

		inline const uint32_t GetStride() const { return m_stride; }
		inline std::vector<BufferElement>& GetElements() { return m_elements; }

	private:
		void CalculateOffsetAndStride()
		{
			size_t offset = 0;
			m_stride = 0;

			for (auto& element : m_elements)
			{
				element.offset = (uint32_t)offset;
				offset += element.size;
				m_stride += element.size;
			}
		}

		std::vector<BufferElement> m_elements;
		uint32_t m_stride = 0;
	};
}