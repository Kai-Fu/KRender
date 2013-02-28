#include "lua_wrapper.h"
#include <KRTCore/entry/Entry.h>
#include <stdexcept>
extern "C" {
	#include "lua5.1/lua.h"
	#include "lua5.1/lualib.h"
	#include "lua5.1/lauxlib.h"
}

extern KRayTracer::KRayTracer_Root* gRoot;

static int LuaWrapper_LoadScene(lua_State *L)
{
	int num_param = lua_gettop(L);
	if (num_param != 1) {
		printf("LoadScene : Invalid input parameters.\n");
		return 0;
	}

	size_t str_len;
	const char* filename = lua_tolstring(L, 1, &str_len);
	
	int success = gRoot->LoadScene(filename) ? 1 : 0;
	lua_pushnumber(L, success);
	
	if (success) {
		SceneStatistic status;
		gRoot->mpSceneLoader->mpScene->GetKDBuildTimeStatistics(status);

		printf("Scene info: T0 %d, T1, %d, L %d, T_L %d, T_B: %d\n",
			(UINT32)status.actual_triangle_count,
			(UINT32)status.leaf_triangle_count,
			(UINT32)status.kd_leaf_count,
			gRoot->mpSceneLoader->mFileLoadingTime,
			status.kd_build_time + status.kd_finialize_time);

		lua_pushnumber(L, (float)status.kd_build_time / 1000.0f);
		lua_pushnumber(L, (UINT32)status.actual_triangle_count);
		lua_pushnumber(L, (UINT32)status.leaf_triangle_count);
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

	int success = gRoot->LoadUpdateFile(filename) ? 1 : 0;
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

	int success = gRoot->SaveScene(filename) ? 1 : 0;
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
	
	gRoot->CloseScene();

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
	UINT32 w = (UINT32)lua_tointeger(L, 1);
	UINT32 h = (UINT32)lua_tointeger(L, 2);
	const char* filename = lua_tolstring(L, 3, &str_len);
	double render_time, output_time;
	int success = gRoot->Render(w, h, filename, render_time, output_time) ? 1 : 0;
	if (success)
		printf("Render time : %f | Output time : %f\n", render_time, output_time);

	lua_pushnumber(L, success);
	lua_pushnumber(L, render_time);

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

	int success = gRoot->SetConstant(name, value) ? 1 : 0;
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

	CameraManager* pManager = CameraManager::GetInstance();
	if (!pManager) {
		printf("No scene is loaded.\n");
		return 0;
	}

	size_t str_len;
	const char* camera_name = lua_tolstring(L, 1, &str_len);
	if (!pManager->SetActiveCamera(camera_name)) {
		printf("Invalid camera name.\n");
	}

	return 0;
}

static int LuaWrapper_ListCamera(lua_State *L)
{
	CameraManager* pManager = CameraManager::GetInstance();
	if (!pManager) {
		printf("No scene is loaded.\n");
		return 0;
	}
	pManager->ResetIter();
	int cnt = 0;
	const char* camera_name = NULL;
	while (camera_name = pManager->GetNextCamera()) {
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

	int success = SetRenderOptions(name, value) ? 1 : 0;
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
	
	gRoot->SetCamera(name, cpos, clookat, cup, cxfov);
	return 0;
ERROR_IN_SetCamera:
	printf("SetCamera : Invalid input parameters.\n");
	return 0;
}

static int LuaWrapper_AddLight(lua_State *L)
{
	if (!gRoot->mpSceneLoader.get()) {
		printf("ERROR: Cannot add light with empty scene.\n");
		return 0;
	}
	// intensity, lightpos, <lightrot, lightsize>
	int num_param = lua_gettop(L);
	if (num_param != 4 && num_param != 2) {
		printf("AddLight : Invalid input parameters.\n");
		return 0;
	}

	KColor intensiy;
	KVec3 pos;
	float xyz_rot[3];
	KVec2 size;

	const char* xfov = NULL;
	if (3 != GetFloatArrayFromTable(L, 1, (float*)&intensiy, 3))
		goto ERROR_IN_AddLight;
	if (3 != GetFloatArrayFromTable(L, 2, (float*)&pos, 3))
		goto ERROR_IN_AddLight;

	pos += gRoot->mpSceneLoader->mImportSceneOffset;
	pos *= gRoot->mpSceneLoader->mImportSceneScale;
	

	if (num_param == 4) {
		if (3 != GetFloatArrayFromTable(L, 3, xyz_rot, 3))
			goto ERROR_IN_AddLight;
		if (2 != GetFloatArrayFromTable(L, 4, (float*)&size, 2))
			goto ERROR_IN_AddLight;
		size[0] *= gRoot->mpSceneLoader->mImportSceneScale;
		size[1] *= gRoot->mpSceneLoader->mImportSceneScale;
	}

	{
		KMatrix4 mat;
		ILightObject* pLight = NULL;
		if (num_param == 2) {
			pLight = LightScheme::GetInstance()->CreateLightSource(POINT_LIGHT_TYPE);
			nvmath::setTransMat(mat, pos);
			pLight->SetLightSpaceMatrix(mat);
		}
		else {
			pLight = LightScheme::GetInstance()->CreateLightSource(RECT_LIGHT_TYPE);
			nvmath::setTransMat(mat, pos);
			 
			nvmath::Quatf mat_rotX( KVec3(1,0,0), xyz_rot[0]/180.0f*nvmath::PI);
			nvmath::Quatf mat_rotY( KVec3(1,0,0), xyz_rot[1]/180.0f*nvmath::PI);
			nvmath::Quatf mat_rotZ( KVec3(1,0,0), xyz_rot[2]/180.0f*nvmath::PI);
			nvmath::Quatf rot = mat_rotX * mat_rotY * mat_rotZ;
			KMatrix4 mat_rot(rot);
			pLight->SetLightSpaceMatrix(mat_rot * mat);
			pLight->SetParam("size_x", &size[0]);
			pLight->SetParam("size_y", &size[1]);
		}

		pLight->SetParam("intensity", &intensiy);
	}
	
	return 0;

ERROR_IN_AddLight:
	printf("AddLight : Invalid input parameters.\n");
	return 0;
}

static int LuaWrapper_ClearLights(lua_State *L)
{
	if (!gRoot->mpSceneLoader.get()) {
		printf("ERROR: Cannot clear light with empty scene.\n");
		return 0;
	}
	LightScheme::GetInstance()->ClearLightSource();
	return 0;
}

void BindLuaFunc()
{
	/* initialize Lua */
	L_S = lua_open();

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
