#include "stringutil.h"

#include <Windows.h>
#include <ctime>

#pragma warning(disable: 4996)

std::string StringUtil::Trim(const std::string & strSrc)
{
	std::string::size_type nFirstIndex = strSrc.find_first_not_of("\r\n\t ");
	std::string::size_type nLastIndex = strSrc.find_last_not_of("\r\n\t ");

	if (nFirstIndex == std::string::npos)
		return std::string("");

	std::string strTrim;
	if ((nLastIndex >= nFirstIndex) && (nFirstIndex != std::string::npos) && (nLastIndex != std::string::npos))
		strTrim = strSrc.substr(nFirstIndex, nLastIndex - nFirstIndex + 1);
	else
		strTrim = strSrc;

	return strTrim;
}

std::string StringUtil::GetModulePath()
{
	char szPath[MAX_PATH];

	if (!GetModuleFileNameA(NULL, szPath, MAX_PATH))
	{
		return "";
	}

	std::string 				strModulePath = szPath;

	std::string::size_type 	idx = strModulePath.find_last_of('\\');
	std::string				strPath = strModulePath.substr(0, idx);

	return strPath;
}

std::string StringUtil::GetCurrentDate()
{
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);

	char			szCurrentTime[32] = { NULL, };
/*
	snprintf(szCurrentTime, 32, "%04d-%02d-%02d"
		, now->tm_year + 1900
		, now->tm_mon + 1
		, now->tm_mday);*/


	sprintf(szCurrentTime, "%04d-%02d-%02d"
		, now->tm_year + 1900
		, now->tm_mon + 1
		, now->tm_mday);

	return szCurrentTime;
}

std::string StringUtil::GetCurrentDateTime()
{
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);

	char			szCurrentTime[32] = { NULL, };
	/*snprintf(szCurrentTime, 32, "%04d-%02d-%02d %02d:%02d:%02d"
		, now->tm_year + 1900
		, now->tm_mon + 1
		, now->tm_mday
		, now->tm_hour
		, now->tm_min
		, now->tm_sec);*/

	sprintf(szCurrentTime, "%04d-%02d-%02d %02d:%02d:%02d"
		, now->tm_year + 1900
		, now->tm_mon + 1
		, now->tm_mday
		, now->tm_hour
		, now->tm_min
		, now->tm_sec);


	return szCurrentTime;
}
