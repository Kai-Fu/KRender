#include <stdio.h>
#include <KRTCore/entry/Entry.h>
#include "lua_wrapper.h"


KRayTracer::KRayTracer_Root* gRoot = NULL;

int main(int arg_cnt, const char* args[])
{
	gRoot = KRayTracer::InitializeKRayTracer();
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

	gRoot->CloseScene();
	UnbindLuaFunc();
	KRayTracer::DestroyKRayTracer();
	return 0;
}