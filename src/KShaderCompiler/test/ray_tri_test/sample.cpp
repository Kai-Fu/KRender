// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "SC_API.h"
#include <string.h>
#include <assert.h>


int main(int argc, char* argv[])
{
	KSC_Initialize();

	FILE* f = NULL;
	fopen_s(&f, "ray_tri_test.ls", "r");
	if (f == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* content = new char[len + 1];
	char* line = content;
	size_t totalLen = 0;

	while (fgets(line, len, f) != NULL) {
		size_t lineLen = strlen(line);
		line += lineLen;
		totalLen += lineLen;
	}

	if (totalLen == 0)
		return -1;
	else {
		content[totalLen] = '\0';

		ModuleHandle hModule = KSC_Compile(content);
		if (!hModule) {
			printf(KSC_GetLastErrorMsg());
			return -1;
		}

		FunctionHandle hFunc = KSC_GetFunctionHandleByName("RayIntersectAnimTriArray", hModule);
		KSC_GetFunctionPtr(hFunc);
	}
	

	return 0;
}

