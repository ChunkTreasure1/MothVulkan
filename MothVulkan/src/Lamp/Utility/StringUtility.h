#pragma once

#include <string>
#include <algorithm>

namespace Utility
{
	std::string ToLower(const std::string& str)
	{
		std::string newStr;
		std::transform(str.begin(), str.end(), newStr.begin(), ::tolower);
	
		return newStr;
	}
}