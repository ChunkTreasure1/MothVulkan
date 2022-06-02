#pragma once

#include <string>
#include <algorithm>
#include <filesystem>

namespace Utility
{
	inline std::string ToLower(const std::string& str)
	{
		std::string newStr(str);
		std::transform(str.begin(), str.end(), newStr.begin(), ::tolower);
	
		return newStr;
	}
}