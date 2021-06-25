#include "Includes.hpp"

void SetRandomConsoleTitle()
{
    char cHexValues[] = 
    { 
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F'
    };

    std::string sTitle = "0x";
    
    for (int i = 0; 8 > i; ++i)
    {
        srand(static_cast<unsigned int>(GetTickCount64() * i));
        sTitle += cHexValues[rand() % 16];
    }

    SetWindowTextA(Global.hConsoleWindow, &sTitle[0]);
    sTitle.clear();
}

void InitFunctions()
{
    HMODULE hMod = GetModuleHandleA("ntdll");
    if (hMod) // NTDLL
    {
        Global.NtProtectVirtualMemory = GetProcAddress(hMod, "NtProtectVirtualMemory");
        Global.NtReadVirtualMemory = GetProcAddress(hMod, "NtReadVirtualMemory");
        Global.NtWriteVirtualMemory = GetProcAddress(hMod, "NtWriteVirtualMemory");
    }
}

int main()
{
    Global.hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!Global.hConsoleOutput) return 1;

    Global.hConsoleWindow = GetConsoleWindow();
    if (!Global.hConsoleWindow) return 1;
    SetRandomConsoleTitle();

    InitFunctions();

    if (!API::GetScriptFile()) return 1;

    SetConsoleTextAttribute(Global.hConsoleOutput, 14Ui16);
    std::cout << "Loading selected script file...\n" << std::endl;
    SetConsoleTextAttribute(Global.hConsoleOutput, 15Ui16);

    Global.pScript = new chaiscript::ChaiScript;
    Global.pScript->add(chaiscript::vector_conversion<std::vector<int>>());
    Global.pScript->add(chaiscript::vector_conversion<std::vector<float>>());

    Global.pScript->add(chaiscript::fun(std::function<void(std::string)>(&APIFunction::_CreateThread)), "CreateThread");
    Global.pScript->add(chaiscript::fun(std::function<void(unsigned int)>(&APIFunction::_Sleep)), "Sleep");
    Global.pScript->add(chaiscript::fun(std::function<bool(int)>(&APIFunction::KeyDown)), "KeyDown");
    Global.pScript->add(chaiscript::fun(std::function<std::string(unsigned int)>(&APIFunction::HexToString)), "HexToString");

    Global.pModule = chaiscript::ModulePtr(new chaiscript::Module); 

    chaiscript::utility::add_class<APIFunction::CConsole>
    (
        *Global.pModule, "GetConsole",
        { chaiscript::constructor<APIFunction::CConsole()>(), chaiscript::constructor<APIFunction::CConsole(APIFunction::CConsole const&)>() },
        {
            { chaiscript::fun(static_cast<void(APIFunction::CConsole::*)(std::string)>(&APIFunction::CConsole::Print)), "Print" },
            { chaiscript::fun(static_cast<void(APIFunction::CConsole::*)()>(&APIFunction::CConsole::Clear)), "Clear" },
            { chaiscript::fun(static_cast<void(APIFunction::CConsole::*)(int)>(&APIFunction::CConsole::Color)), "Color" },
            { chaiscript::fun(static_cast<void(APIFunction::CConsole::*)()>(&APIFunction::CConsole::WaitForInput)), "WaitForInput" }
        }
    );

    chaiscript::utility::add_class<APIFunction::CProcess>
    (
        *Global.pModule, "GetModule",
        { chaiscript::constructor<APIFunction::CModule()>(), chaiscript::constructor<APIFunction::CModule(APIFunction::CModule const&)>() },
        {
            { chaiscript::fun(static_cast<unsigned int(APIFunction::CModule::*)()>(&APIFunction::CModule::GetBase)), "GetBase" },
            { chaiscript::fun(static_cast<unsigned int(APIFunction::CModule::*)()>(&APIFunction::CModule::GetSize)), "GetSize" },
            { chaiscript::fun(static_cast<void(APIFunction::CModule::*)(HANDLE)>(&APIFunction::CModule::SetHandle)), "SetHandle" },
            { chaiscript::fun(static_cast<bool(APIFunction::CModule::*)(std::string)>(&APIFunction::CModule::Find)), "Find" },
            { chaiscript::fun(static_cast<unsigned int(APIFunction::CModule::*)(std::string)>(&APIFunction::CModule::SigScan)), "SigScan" }
        }
    );

    chaiscript::utility::add_class<APIFunction::CProcess>
    (
        *Global.pModule, "GetProcess",
        { chaiscript::constructor<APIFunction::CProcess()>(), chaiscript::constructor<APIFunction::CProcess(APIFunction::CProcess const&)>() },
        {
            { chaiscript::fun(static_cast<void(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::SetID)), "SetID" },
            { chaiscript::fun(static_cast<unsigned int(APIFunction::CProcess::*)()>(&APIFunction::CProcess::GetID)), "GetID" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(std::string)>(&APIFunction::CProcess::Find)), "Find" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(APIFunction::CModule&, std::string)>(&APIFunction::CProcess::FindModule)), "FindModule" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::_OpenHandle)), "OpenHandle" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)()>(&APIFunction::CProcess::_CloseHandle)), "CloseHandle" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)()>(&APIFunction::CProcess::_Kill)), "Kill" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int, std::vector<int>)>(&APIFunction::CProcess::WriteByte)), "WriteByte" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int, bool)>(&APIFunction::CProcess::WriteBool)), "WriteBool" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int, std::vector<int>)>(&APIFunction::CProcess::WriteInteger)), "WriteInteger" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int, std::vector<float>)>(&APIFunction::CProcess::WriteFloat)), "WriteFloat" },
            { chaiscript::fun(static_cast<unsigned char(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::ReadByte)), "ReadByte" },
            { chaiscript::fun(static_cast<bool(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::ReadBool)), "ReadBool" },
            { chaiscript::fun(static_cast<int(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::ReadInteger)), "ReadInteger" },
            { chaiscript::fun(static_cast<float(APIFunction::CProcess::*)(unsigned int)>(&APIFunction::CProcess::ReadFloat)), "ReadFloat" }
        }
    );

    Global.pScript->add(Global.pModule);

    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iBlue), "TCOL_BLUE");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iGreen), "TCOL_GREEN");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iCyan), "TCOL_CYAN");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iRed), "TCOL_RED");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iPink), "TCOL_PINK");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iYellow), "TCOL_YELLOW");
    Global.pScript->set_global(chaiscript::const_var(APIFunction::ConsoleColors::iWhite), "TCOL_WHITE");

    Global.pScript->set_global(chaiscript::const_var(APIFunction::ProcessAccess::uAllAccess), "PROCESS_ALL_ACCESS");

    Global.pScript->eval_file(API::cFilePath);

    int iExit = getchar();
    return 0;
}