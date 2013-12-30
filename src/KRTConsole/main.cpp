#include <stdio.h>
#include "lua_wrapper.h"
#include <KRTCore/api/KRT_API.h>

int main(int arg_cnt, const char* args[])
{
	if (!KRT_Initialize())
		return -1;
	BindLuaFunc();
	RunLuaCommandFromFile("startup.lua");

	if (arg_cnt > 1) {
		for (int i = 1; i < arg_cnt; ++i)
		RunLuaCommandFromFile(args[i]);
	}
	else {
		char command[1024];
		printf("> ");
		while (!GetQuitFlag()) {
			fgets(command, 1024, stdin);
			RunLuaCommand(command);
			printf("> ");
		}
	}

	KRT_CloseScene();
	UnbindLuaFunc();
	KRT_Destory();
	return 0;
}