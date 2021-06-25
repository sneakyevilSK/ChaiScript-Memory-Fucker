#include "Includes.hpp"

namespace API
{
	char cFilePath[512];

	bool GetScriptFile()
	{
		OPENFILENAME oGetFileName;
		memset(&oGetFileName, 0, sizeof(oGetFileName));

		oGetFileName.lStructSize = sizeof(oGetFileName);
		oGetFileName.hwndOwner = NULL;
		oGetFileName.lpstrFile = cFilePath;
		oGetFileName.lpstrFile[0] = '\0';
		oGetFileName.nMaxFile = sizeof(cFilePath);
		oGetFileName.lpstrFilter = "Script\0*.mscript\0";
		oGetFileName.nFilterIndex = 1;
		oGetFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		return static_cast<bool>(GetOpenFileNameA(&oGetFileName));
	}
}