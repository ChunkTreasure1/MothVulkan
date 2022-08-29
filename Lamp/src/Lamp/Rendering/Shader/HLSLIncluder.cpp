#include "lppch.h"
#include "HLSLIncluder.h"

#include "Lamp/Core/Base.h"
#include "Lamp/Log/Log.h"

namespace Lamp
{
	HRESULT HLSLIncluder::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
	{
		static IDxcUtils* utils = nullptr;
		if (!utils)
		{
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
			if (FAILED(utils->CreateDefaultIncludeHandler(&s_defaultIncludeHandler)))
			{
				LP_CORE_ASSERT(false, "Failed to create default hlsl includer!");
			}
		}

		const std::filesystem::path path = pFilename;
		
		std::ifstream stream;
		stream.open(path);

		if (!stream.is_open())
		{
			LP_CORE_ERROR("Failed to read file {0}", path.string().c_str());
			return S_FALSE;
		}

		std::stringstream buffer;
		buffer << stream.rdbuf();
		stream.close();

		std::string data = buffer.str();

		IDxcBlobEncoding* encoding;
		utils->CreateBlob(data.data(), (uint32_t)data.size(), CP_UTF8, &encoding);

		*ppIncludeSource = encoding;
		return S_OK;
	}
}