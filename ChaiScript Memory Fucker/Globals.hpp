#pragma once

class CGlobalVariables
{
public:
	HANDLE hConsoleOutput = nullptr;
	HWND hConsoleWindow = nullptr;
	chaiscript::ChaiScript* pScript = nullptr;
	chaiscript::ModulePtr pModule = nullptr;

	void* NtProtectVirtualMemory = nullptr;
	void* NtReadVirtualMemory = nullptr;
	void* NtWriteVirtualMemory = nullptr;
};

extern CGlobalVariables Global;