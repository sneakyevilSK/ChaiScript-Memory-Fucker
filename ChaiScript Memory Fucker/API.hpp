#pragma once

namespace API
{
	extern char cFilePath[512];

	bool GetScriptFile();
}

namespace APIFunction
{
	namespace ConsoleColors
	{
		static int iBlue = 9;
		static int iGreen = 10;
		static int iCyan = 11;
		static int iRed = 12;
		static int iPink = 13;
		static int iYellow = 14;
		static int iWhite = 15;
	}

	namespace ProcessAccess
	{
		static unsigned int uAllAccess = PROCESS_ALL_ACCESS;
	}

	class CConsole
	{
	public:
		void Print(std::string sText)
		{
			std::cout << sText;
		}

		void Clear()
		{
			CONSOLE_SCREEN_BUFFER_INFO cScreenInfo;
			GetConsoleScreenBufferInfo(Global.hConsoleOutput, &cScreenInfo);

			COORD cTopLeft = { 0, 0 };
			DWORD dWritten;
			FillConsoleOutputCharacterA(Global.hConsoleOutput, ' ', cScreenInfo.dwSize.X * cScreenInfo.dwSize.Y, cTopLeft, &dWritten);
			FillConsoleOutputAttribute(Global.hConsoleOutput, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, cScreenInfo.dwSize.X * cScreenInfo.dwSize.Y, cTopLeft, &dWritten);
			SetConsoleCursorPosition(Global.hConsoleOutput, cTopLeft);
		}

		void Color(int iColor)
		{
			if (0 >= iColor || iColor > 15) iColor = 15;

			SetConsoleTextAttribute(Global.hConsoleOutput, static_cast<unsigned short>(iColor));
		}

		void WaitForInput()
		{
			int iDummy = getchar();
		}
	};

	class CMemory
	{
	public:

	};

	class CModule
	{
	public:
		unsigned int uBase = 0U;
		unsigned int uSize = 0U;

		unsigned int GetBase() { return uBase; }
		unsigned int GetSize() { return uSize; }

		HANDLE hProcess = nullptr;
		void SetHandle(HANDLE hSet) { hProcess = hSet; }

		bool Find(std::string sName)
		{
			if (!hProcess) return false;

			HMODULE hModules[1024];
			DWORD dNeeded;
			if (!K32EnumProcessModules(hProcess, hModules, sizeof(hModules), &dNeeded)) return false;

			uBase = 0U;
			uSize = 0U;

			char cModuleName[MAX_PATH];
			for (unsigned int i = 0; (dNeeded / sizeof(HMODULE)) > i; ++i)
			{
				if (!K32GetModuleFileNameExA(hProcess, hModules[i], cModuleName, sizeof(cModuleName))) continue;

				const char* pLastDelimer = strrchr(cModuleName, '\\');
				if (!pLastDelimer) continue;

				pLastDelimer = reinterpret_cast<const char*>(reinterpret_cast<unsigned int>(pLastDelimer) + 0x1);
				if (strcmp(pLastDelimer, &sName[0])) continue;

				MODULEINFO mInfo;
				if (!K32GetModuleInformation(hProcess, hModules[i], &mInfo, sizeof(mInfo))) continue;

				uBase = reinterpret_cast<unsigned int>(mInfo.lpBaseOfDll);
				uSize = static_cast<unsigned int>(mInfo.SizeOfImage);
				break;
			}

			return uBase != 0U && uSize > 0U;
		}

#define INRANGE(x, a, b) (x >= a && x <= b) 
#define getBits(x) (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

		unsigned int SigScan(std::string sSig)
		{
			if (!hProcess || uBase == 0U) return 0U;

			unsigned char* pData = new unsigned char[uSize];

			unsigned int uReturn = 0U;
			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtReadVirtualMemory)(hProcess, reinterpret_cast<void*>(uBase), pData, static_cast<unsigned long>(uSize), nullptr) == NT_SUCCESS;
			if (bSuccess)
			{
				const char* cSig = &sSig[0];

				size_t sLength = strlen(cSig);
				size_t sAllocSize = sLength >> 1;

				PBYTE pPatternAlloc = new BYTE[sAllocSize];
				PBYTE pMaskAlloc = new BYTE[sAllocSize];
				PBYTE pPattern = pPatternAlloc;
				PBYTE pMask = pMaskAlloc;

				sLength = 0;
				while (*cSig)
				{
					if (*cSig == ' ') cSig++;
					if (!*cSig) break;
					if (*(PBYTE)cSig == (BYTE)'\?')
					{
						*pPattern++ = 0;
						*pMask++ = '?';
						cSig += ((*(PWORD)cSig == (WORD)'\?\?') ? 2 : 1);
					}
					else
					{
						*pPattern++ = getByte(cSig);
						*pMask++ = 'x';
						cSig += 2;
					}
					sLength++;
				}

				*pMask = 0;
				pPattern = pPatternAlloc;
				pMask = pMaskAlloc;

				auto IsMatch = [](const PBYTE pAddress, const PBYTE pPattern, const PBYTE pMask) -> bool
				{
					size_t s = 0;
					while (pAddress[s] == pPattern[s] || pMask[s] == (BYTE)'?')
					{
						if (!pMask[++s]) return true;
					}
					return false;
				};

				PBYTE pStart = reinterpret_cast<PBYTE>(pData);
				for (DWORD i = 0; uSize - sLength > i; ++i)
				{
					if (IsMatch(pStart + i, pPatternAlloc, pMaskAlloc))
					{
						uReturn = static_cast<unsigned int>(uBase + i);
						break;
					}
				}

				delete[] pPatternAlloc;
				delete[] pMaskAlloc;
			}
			
			delete[] pData;
			return uReturn;
		}
	};

	class CProcess
	{
	public:
		unsigned int uID = 0U;
		HANDLE hHandle = nullptr;

		void SetID(unsigned int uSet) { uID = uSet; }
		unsigned int GetID() { return uID; }

		bool Find(std::string sName)
		{
			uID = 0U;

			HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnapShot)
			{
				PROCESSENTRY32 pEntry32;
				pEntry32.dwSize = sizeof(pEntry32);
				if (Process32First(hSnapShot, &pEntry32))
				{
					while (Process32Next(hSnapShot, &pEntry32))
					{
						if (strcmp(pEntry32.szExeFile, &sName[0])) continue;

						uID = pEntry32.th32ProcessID;
						break;
					}
				}

				CloseHandle(hSnapShot);
			}

			return uID != 0U;
		}

		bool FindModule(CModule& Module, std::string sName)
		{
			Module.SetHandle(hHandle);
			return Module.Find(sName);
		}

		bool _OpenHandle(unsigned int uAccess)
		{
			if (uID == 0U) return false;

			hHandle = OpenProcess(uAccess, 0, uID);
			
			return hHandle != nullptr;
		}

		bool _CloseHandle()
		{
			if (!hHandle) return false;

			return static_cast<bool>(CloseHandle(hHandle));
		}

		bool _Kill()
		{
			if (!hHandle) return false;

			return static_cast<bool>(TerminateProcess(hHandle, 0U));
		}

		// Memory
		void* pAddressProtection = nullptr;
		unsigned long uSizeProtection = 0UL;
		unsigned long uLastProtection = 0UL;

		bool Protect(unsigned int uAddress, unsigned int uSize, unsigned int uProtection)
		{
			pAddressProtection = reinterpret_cast<void*>(uAddress);
			uSizeProtection = static_cast<unsigned long>(uSize);
			return reinterpret_cast<long(__stdcall*)(HANDLE, void**, unsigned long*, unsigned long, unsigned long*)>(Global.NtProtectVirtualMemory)(hHandle, &pAddressProtection, &uSizeProtection, static_cast<unsigned long>(uProtection), &uLastProtection) == NT_SUCCESS;
		}

		bool WriteByte(unsigned int uAddress, std::vector<int> vVector)
		{
			if (!hHandle) return false;

			unsigned int uSize = vVector.size();
			if (!Protect(uAddress, uSize, PAGE_EXECUTE_READWRITE)) return false;

			static std::vector<unsigned char> vBytes;
			vBytes.clear();
			for (unsigned int i = 0; uSize > i; ++i) vBytes.emplace_back(static_cast<unsigned char>(vVector[i]));

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtWriteVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), vBytes.data(), static_cast<unsigned long>(uSize), nullptr) == NT_SUCCESS;
			Protect(uAddress, uSize, uLastProtection);

			return bSuccess;
		}

		bool WriteBool(unsigned int uAddress, bool bValue)
		{
			if (!hHandle) return false;

			if (!Protect(uAddress, 1U, PAGE_EXECUTE_READWRITE)) return false;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtWriteVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), &bValue, 1ULL, nullptr) == NT_SUCCESS;
			Protect(uAddress, 1U, uLastProtection);

			return bSuccess;
		}

		bool WriteInteger(unsigned int uAddress, std::vector<int> vVector)
		{
			if (!hHandle) return false;

			unsigned int uSize = vVector.size() * 0x4;
			if (!Protect(uAddress, uSize, PAGE_EXECUTE_READWRITE)) return false;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtWriteVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), vVector.data(), static_cast<unsigned long>(uSize), nullptr) == NT_SUCCESS;
			Protect(uAddress, uSize, uLastProtection);

			return bSuccess;
		}

		bool WriteFloat(unsigned int uAddress, std::vector<float> vVector)
		{
			if (!hHandle) return false;

			unsigned int uSize = vVector.size() * 0x4;
			if (!Protect(uAddress, uSize, PAGE_EXECUTE_READWRITE)) return false;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtWriteVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), vVector.data(), static_cast<unsigned long>(uSize), nullptr) == NT_SUCCESS;
			Protect(uAddress, uSize, uLastProtection);

			return bSuccess;
		}

		unsigned char ReadByte(unsigned int uAddress)
		{
			static unsigned char uValue = 0Ui8;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtReadVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), &uValue, 1UL, nullptr) == NT_SUCCESS;
			if (bSuccess) return uValue;

			return 0Ui8;
		}

		bool ReadBool(unsigned int uAddress)
		{
			if (ReadByte(uAddress) == 0Ui8) return false;

			return true;
		}

		int ReadInteger(unsigned int uAddress)
		{
			static int iValue = 0;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtReadVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), &iValue, 4UL, nullptr) == NT_SUCCESS;
			if (bSuccess) return iValue;

			return 0;
		}

		float ReadFloat(unsigned int uAddress)
		{
			static float fValue = 0;

			bool bSuccess = reinterpret_cast<long(__stdcall*)(HANDLE, void*, void*, unsigned long, unsigned long*)>(Global.NtReadVirtualMemory)(hHandle, reinterpret_cast<void*>(uAddress), &fValue, 4UL, nullptr) == NT_SUCCESS;
			if (bSuccess) return fValue;

			return 0.f;
		}
	};

	class CThread
	{
	public:
		std::function<void()> fFunction = nullptr;
	};

	static DWORD __stdcall ClassThread(void* pParam)
	{
		CThread* pThread = reinterpret_cast<CThread*>(pParam);

		pThread->fFunction();

		delete pThread;
		return 0;
	}

	static void _CreateThread(std::string sFunction)
	{
		CThread* pThread = new CThread;	
		pThread->fFunction = Global.pScript->eval<std::function<void()>>(sFunction);
		if (pThread->fFunction)
		{
			CreateThread(0, 0, ClassThread, pThread, 0, 0);
			return;
		}

		delete pThread;
	}

	static void _Sleep(unsigned int iMS)
	{
		Sleep(iMS);
	}

	static bool KeyDown(int iKey)
	{
		return GetAsyncKeyState(iKey) & 0x8000;
	}

	static std::string HexToString(unsigned int uValue)
	{
		static char cReturn[12];
		sprintf_s(cReturn, sizeof(cReturn), "0x%X", uValue);
		return cReturn;
	}
}