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
		for (int i = 1; i < arg_cnt; ++i) {
			std::string temp_cmd = "Run(\"";
			temp_cmd += args[i];
			temp_cmd += "\")";

			std::string temp_cmd1;
			for (int i = 0; i < temp_cmd.length(); ++i) {
				if (temp_cmd[i] == '\\')
					temp_cmd1 += "\\\\";
				else
					temp_cmd1 += temp_cmd[i];
		   }
			RunLuaCommand(temp_cmd1.c_str());
		}
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