#pragma once
#include <string>

namespace StringUtility {
	//string‚ðwstring‚É•Ï‚¦‚é
	std::wstring ConvertString(const std::string& str);
	//wstring‚ðstring‚É•Ï‚¦‚é
	std::string ConvertString(const std::wstring& str);
}

