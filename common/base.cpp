#include "stdafx.h"
#include "base.h"
#include <boost/filesystem.hpp>


bool Run_Exe(const std::string& name
	, const std::string& commandLine)
{
	/*
	SHELLEXECUTEINFOA ExecuteInfo;
	memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));

	ExecuteInfo.cbSize = sizeof(ExecuteInfo);
	ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ExecuteInfo.hwnd = 0;
	ExecuteInfo.lpVerb = "runas";
	ExecuteInfo.lpFile = name.c_str();
	if (!commandLine.empty())
	ExecuteInfo.lpParameters = commandLine.c_str();
	ExecuteInfo.lpDirectory = 0;
	ExecuteInfo.nShow = SW_SHOW;
	ExecuteInfo.hInstApp = 0;

	return ShellExecuteExA(&ExecuteInfo) == TRUE;
	*/
	try{
		STARTUPINFOA si = { sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi = { 0 };

		boost::filesystem::path p(name);
		std::string filename = p.string();
		if (!commandLine.empty())
			filename += " " + commandLine;

		BOOL ret = CreateProcessA(
			NULL,
			(char*)filename.c_str(),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			p.parent_path().string().c_str(),
			&si,
			&pi
			);
		return TRUE == ret;
	}
	catch (...){
		return false;
	}
}