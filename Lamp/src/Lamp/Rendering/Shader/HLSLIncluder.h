#pragma once

#include <dxc/dxcapi.h>
#include <Windows.h>

namespace Lamp
{
	class HLSLIncluder : public IDxcIncludeHandler
	{
	public:
		HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;
		HRESULT QueryInterface(const IID& riid, void** ppvObject) override { return s_defaultIncludeHandler->QueryInterface(riid, ppvObject); }
	
		ULONG AddRef() override { return 0; }
		ULONG Release() override { return 0; }

	private:
		inline static IDxcIncludeHandler* s_defaultIncludeHandler = nullptr;
	};
}