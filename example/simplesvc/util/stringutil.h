#pragma once

#include <string>

class StringUtil {
public:
	//map<string, string>
	static std::string				Trim(const std::string& strSrc);
	static std::string				GetModulePath();

	static std::string				GetCurrentDate();
	static std::string				GetCurrentDateTime();
	static std::string				Format(const std::string fmt, ...);
};

