
#include "stdafx.h"

#include <DbgHelp.h>

#pragma comment (lib, "dbghelp.lib")

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("\n	*** OffsetToSource by Sasha Goldshtein (blog.sashag.net), (C) 2013 ***\n\n");
		printf("	Usage: OffsetToSource module[.exe|.dll][!function]+0x<offset>\n\n");
		printf("		Example: OffsetToSource myapp!main+0xd0\n");
		printf("		Example: OffsetToSource myapp+0x441d0\n");
		return 2;
	}

	const HANDLE hProcess = GetCurrentProcess();

	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_LOAD_LINES);

	if (FALSE == SymInitialize(hProcess, NULL, FALSE))
	{
		printf("*** Error initializing symbol engine: 0x%x\n", GetLastError());
		return 1;
	}

	CHAR symbolPath[2048], path[2048];
	GetEnvironmentVariableA("PATH", path, ARRAYSIZE(symbolPath));
	GetEnvironmentVariableA("_NT_SYMBOL_PATH", symbolPath, ARRAYSIZE(symbolPath));
	strcat_s(path, ";.;");
	strcat_s(path, symbolPath);
	SymSetSearchPath(hProcess, path);

	//Parse argv[1] to obtain the module name, symbol name, and offset
	//		Example format:		module!Class::Method+0x40
	//		Another option:		module+0x1000
	BOOL symbolNameSpecified;
	CHAR moduleName[200], symName[200];
	DWORD offset = 0;
	CHAR* bang = strchr(argv[1], '!');
	if (bang != NULL)
	{
		strncpy_s(moduleName, argv[1], bang - argv[1]);
		CHAR* plus = strchr(bang, '+');
		if (plus != NULL)
		{
			strncpy_s(symName, bang + 1, plus - bang - 1);
			sscanf_s(plus + 1, "0x%x", &offset);
		}
		else
		{
			strcpy_s(symName, bang + 1);
		}
		symbolNameSpecified = TRUE;
	}
	else
	{
		CHAR* plus = strchr(argv[1], '+');
		if (plus == NULL)
		{
			printf("*** Invalid input: %s\n", argv[1]);
			return 1;
		}
		strncpy_s(moduleName, argv[1], plus - argv[1]);
		sscanf_s(plus + 1, "0x%x", &offset);
		symbolNameSpecified = FALSE;
	}

	BOOL nakedName = strstr(moduleName, ".dll") == NULL && strstr(moduleName, ".exe") == NULL;
	if (nakedName)
	{
		strcat_s(moduleName, ".dll");
	}
	DWORD64 moduleBase;
	while (0 == (moduleBase = SymLoadModule64(hProcess, NULL, moduleName, NULL, 0, 0)))
	{
		if (nakedName)
		{
			strcpy(strstr(moduleName, ".dll"), ".exe");
			nakedName = FALSE;
			continue;
		}
		printf("*** Error loading symbols: 0x%x\n", GetLastError());
		return 1;
	}

	DWORD64 symbolAddress;
	if (symbolNameSpecified)
	{
		ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;
		if (FALSE == SymFromName(hProcess, symName, pSymbol))
		{
			printf("*** Error retrieving symbol information for symbol %s: 0x%x\n",
				argv[1], GetLastError());
			return 1;
		}
		symbolAddress = pSymbol->Address + offset;
	}
	else
	{
		symbolAddress = moduleBase + offset;
	}

	DWORD displacement;
	IMAGEHLP_LINE64 line;
	RtlZeroMemory(&line, sizeof(line));
	line.SizeOfStruct = sizeof(line);
	if (FALSE == SymGetLineFromAddr64(hProcess, symbolAddress, &displacement, &line))
	{
		printf("*** Error retrieving source line for %s: 0x%x\n", argv[1], GetLastError());
		return 1;
	}

	printf("%s [0x%I64x] = %s line %d (+0x%x)\n", argv[1], symbolAddress,
		line.FileName, line.LineNumber, displacement);

	SymCleanup(hProcess);

	return 0;
}

