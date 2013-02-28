#pragma once

void BindLuaFunc();

void UnbindLuaFunc();

void RunLuaCommand(const char* cmd);

void RunLuaCommandFromFile(const char* filename);

bool GetQuitFlag();