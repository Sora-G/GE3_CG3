#pragma once
#include <string>

namespace StringUtility {
	//stringをwstringに変える
	std::wstring ConvertString(const std::string& str);
	//wstringをstringに変える
	std::string ConvertString(const std::wstring& str);
}

