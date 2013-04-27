#include "lua_wrapper.h"
#include <KRTCore/api/KRT_API.h>
#include <stdexcept>
extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}


static int LuaWrapper_LoadScene(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 1) {
		printf("LoadScene : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* filename = lua_tolstring(L, 1, &str_len);
	KRT_SceneStatistic statistic;
	int success = KRT_LoadScene(filename, statistic) ? 1 : 0;
	lua_pushnumber(L, success);
	
	if (success) {
		
			printf("Scene info: T0 %d, T1, %d, L %d, T_B: %d\n",
				statistic.actual_triangle_count,
				statistic.leaf_triangle_count,
				statistic.kd_leaf_count,
				statistic.kd_build_time + statistic.kd_finialize_time);

		lua_pushnumber(L, (float)statistic.kd_build_time / 1000.0f);
		lua_pushnumber(L, (lua_Number)statistic.actual_triangle_count);
		lua_pushnumber(L, (lua_Number)statistic.leaf_triangle_count);
	}
	else {
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
	}
	
	return 4;
}

static int LuaWrapper_UpdateScene(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 1) {
		printf("UpdateScene : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* filename = lua_tolstring(L, 1, &str_len);

	int success = KRT_LoadUpdate(filename) ? 1 : 0;
	lua_pushnumber(L, success);

	if (success)
		printf("Scene updated.\n");

	return 1;
}


static int LuaWrapper_SaveScene(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 1) {
		printf("SaveScene : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* filename = lua_tolstring(L, 1, &str_len);

	int success = KRT_SaveScene(filename) ? 1 : 0;
	lua_pushnumber(L, success);

	return 1;
}

static int LuaWrapper_CloseScene(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 0) {
		printf("CloseScene : Invalid input parameters.\n");
		return 0;
	}
	
	KRT_CloseScene();

	return 0;
}

static int LuaWrapper_Render(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 3) {
		printf("Render : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	unsigned w = (unsigned)lua_tointeger(L, 1);
	unsigned h = (unsigned)lua_tointeger(L, 2);
	const char* filename = lua_tolstring(L, 3, &str_len);
	KRT_RenderStatistic stat;
	int success = KRT_RenderToImage(w, h, kRGB_8, filename, stat) ? 1 : 0;
	if (success)
		printf("Render time : %f\n", stat.render_time);

	lua_pushnumber(L, success);
	lua_pushnumber(L, stat.render_time);

	return 2;
}

static int LuaWrapper_SetConstant(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 2) {
		printf("SetConstant : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* name = lua_tolstring(L, 1, &str_len);
	const char* value = lua_tolstring(L, 2, &str_len);

	int success = KRT_SetConstant(name, value) ? 1 : 0;
	lua_pushnumber(L, success);

	return 1;
}

static int LuaWrapper_SetActiveCamera(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 1) {
		printf("SetActiveCamera : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* camera_name = lua_tolstring(L, 1, &str_len);
	if (!KRT_SetActiveCamera(camera_name)) {
		printf("Invalid camera name.\n");
	}

	return 0;
}

static int LuaWrapper_ListCamera(lua_State *L)
{
	int cnt = 0;
	const char* camera_name = NULL;
	unsigned camCnt = KRT_GetCameraCount();
	for (unsigned i = 0; i < camCnt; ++i) {
		camera_name = KRT_GetCameraName(i);
		lua_pushlstring(L, camera_name, strlen(camera_name));
		++cnt;
	}
	return cnt;
}


static int LuaWrapper_SetRenderOptions(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 2) {
		printf("SetRenderOptions : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* name = lua_tolstring(L, 1, &str_len);
	const char* value = lua_tolstring(L, 2, &str_len);

	int success = KRT_SetRenderOption(name, value) ? 1 : 0;
	lua_pushnumber(L, success);

	return 1;
}

static lua_State* L_S;
static int LuaQuitFlag = 0;

static int LuaWrapper_Quit(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 0) {
		printf("Quit : Invalid input parameters.\n");
		return 0;
	}
	LuaQuitFlag = 1;
	return 0;
}

bool GetQuitFlag()
{
	return LuaQuitFlag ? true : false;
}

static int GetFloatArrayFromTable(lua_State* L, int table_index, float* pArray, int buf_len)
{
	if(!lua_istable(L,table_index))
    {
        return 0;
    }

    lua_pushnil(L);  /* first key */
	int idx = 0;
	size_t str_len;
    while (lua_next(L, table_index) != 0 && idx < buf_len) {
        /* uses 'key' (at index -2) and 'value' (at index -1) */ 
        //add your own code to get the key and value
		const char* value_str = lua_tolstring(L, -1, &str_len);
		sscanf_s(value_str, "%f", &pArray[idx++]);
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
	return idx;
}

static int LuaWrapper_SetCamera(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 5) {
		printf("SetCamera : Invalid input parameters.\n");
		return 0;
	}
	
	float cpos[3], clookat[3], cup[3], cxfov;
	const char* xfov = NULL;
	size_t str_len;
	const char* name = lua_tolstring(L, 1, &str_len);
	if (3 != GetFloatArrayFromTable(L, 2, cpos, 3))
		goto ERROR_IN_SetCamera;
	if (3 != GetFloatArrayFromTable(L, 3, clookat, 3))
		goto ERROR_IN_SetCamera;
	if (3 != GetFloatArrayFromTable(L, 4, cup, 3))
		goto ERROR_IN_SetCamera;
	
	xfov = lua_tolstring(L, 5, &str_len);
	if (1 != sscanf_s(xfov, "%f", &cxfov))
		goto ERROR_IN_SetCamera;
	
	KRT_SetCamera(name, cpos, clookat, cup, cxfov);
	return 0;
ERROR_IN_SetCamera:
	printf("SetCamera : Invalid input parameters.\n");
	return 0;
}

static int LuaWrapper_AddLight(lua_State *L)
{
	// lightpos, <lightrot>
	int num_param = lua_gettop(L);
	if (num_param != 2 && num_param != 1) {
		printf("AddLight : Invalid input parameters.\n");
		return 0;
	}

	float pos[3];
	float xyz_rot[3];

	if (3 != GetFloatArrayFromTable(L, 1, (float*)&pos, 3))
		goto ERROR_IN_AddLight;

	if (num_param == 2) {
		if (3 != GetFloatArrayFromTable(L, 2, xyz_rot, 3))
			goto ERROR_IN_AddLight;
	}

	int idx = (int)KRT_AddLightSource(pos, xyz_rot);
	lua_pushnumber(L, idx);
	return 0;

ERROR_IN_AddLight:
	printf("AddLight : Invalid input parameters.\n");
	return 0;
}

static int LuaWrapper_ClearLights(lua_State *L)
{
	KRT_DeleteAllLights();
	return 0;
}

void BindLuaFunc()
{
	/* initialize Lua */
	L_S = luaL_newstate();

	/* load Lua base libraries */
	luaL_openlibs(L_S);

	/* register our function */
	lua_register(L_S, "LoadScene", LuaWrapper_LoadScene);
	lua_register(L_S, "UpdateScene", LuaWrapper_UpdateScene);
	lua_register(L_S, "SaveScene", LuaWrapper_SaveScene);
	lua_register(L_S, "CloseScene", LuaWrapper_CloseScene);
	lua_register(L_S, "Render", LuaWrapper_Render);
	lua_register(L_S, "SetConstant", LuaWrapper_SetConstant);
	lua_register(L_S, "ListCamera", LuaWrapper_ListCamera);
	lua_register(L_S, "SetActiveCamera", LuaWrapper_SetActiveCamera);
	lua_register(L_S, "SetCamera", LuaWrapper_SetCamera);
	lua_register(L_S, "AddLight", LuaWrapper_AddLight);
	lua_register(L_S, "ClearLights", LuaWrapper_ClearLights);
	lua_register(L_S, "SetRenderOptions", LuaWrapper_SetRenderOptions);
	lua_register(L_S, "Quit", LuaWrapper_Quit);

}

void UnbindLuaFunc()
{
	lua_close(L_S);
}

void RunLuaCommand(const char* cmd)
{
	if (0 != luaL_loadstring(L_S, cmd)) {
		printf("Bad command string.\n");
		return;
	}
	if (0 != lua_pcall(L_S, 0, LUA_MULTRET, 0)) {
		printf("%s\n", lua_tostring(L_S, -1));
		lua_pop(L_S, 1); 
	}
	
}

void RunLuaCommandFromFile(const char* filename)
{
	if (0 != luaL_loadfile(L_S, filename)) {
		printf("Bad command string.\n");
		return;
	}
	if (0 != lua_pcall(L_S, 0, LUA_MULTRET, 0)) {
		printf("%s\n", lua_tostring(L_S, -1));
		lua_pop(L_S, 1); 
	}
}
