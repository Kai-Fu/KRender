#include "constants.h"
#include "../base/base_header.h"
#include "../os/api_wrapper.h"
#include <string>

// Global settings
UINT32 RT_THREAD_STACK_SIZE = 100000;
UINT32 LEAF_TRIANGLE_CNT = 32;
UINT32 MAX_KD_DEPTH = 40;
float  COLOR_DIFF_THRESH_HOLD = 0.05f;
UINT32 CPU_COUNT = 0;
UINT32 MAX_REFLECTION_BOUNCE = 10;
UINT32 USE_TEX_MAP = 1;

#ifdef __GNUC__
#define sscanf_s(str, format, ref, buf_size) sscanf(str, format, ref)
#endif

// Render options
UINT32 PIXEL_SAMPLE_CNT_EVAL = 3;
UINT32 PIXEL_SAMPLE_CNT_MORE = 4;
UINT32 PIXEL_SAMPLE_CNT_EDGE = 4;
UINT32 AREA_LIGHT_SAMP_CNT = 10;
UINT32 ENABLE_DOF = 0;
UINT32 ENABLE_MB = 1;

#define CLAMP(value, min, max) {if (value < min) value = min;  if (value > max) value = max;}
bool SetGlobalConstant(const char* name, const char* value)
{
	std::string var = name;
	// Convert the variable name to upper case
	std::transform(var.begin(), var.end(), var.begin(), toupper);
	if (var == "RT_THREAD_STACK_SIZE") {
		sscanf_s(value, "%d", &RT_THREAD_STACK_SIZE, sizeof(UINT32));
		CLAMP(RT_THREAD_STACK_SIZE, 10000, 500000);
	}
	else if (var == "LEAF_TRIANGLE_CNT") {
		sscanf_s(value, "%d", &LEAF_TRIANGLE_CNT, sizeof(UINT32));
		CLAMP(LEAF_TRIANGLE_CNT, 10, 400);
	}
	else if (var == "MAX_KD_DEPTH") {
		sscanf_s(value, "%d", &MAX_KD_DEPTH, sizeof(UINT32));
		CLAMP(MAX_KD_DEPTH, 10, 400);
	}
	else if (var == "COLOR_DIFF_THRESH_HOLD") {
		sscanf_s(value, "%f", &COLOR_DIFF_THRESH_HOLD, sizeof(float));
		CLAMP(COLOR_DIFF_THRESH_HOLD, 0.001f, 0.3f);
	}
	else if (var == "CPU_COUNT") {
		sscanf_s(value, "%d", &CPU_COUNT, sizeof(UINT32));
	}
	else if (var == "USE_TEX_MAP") {
		sscanf_s(value, "%d", &USE_TEX_MAP, sizeof(UINT32));
	}
	else {
		return false;
	}

	return true;
}

bool SetRenderOptions(const char* name, const char* value)
{
	std::string var = name;
	// Convert the variable name to upper case
	std::transform(var.begin(), var.end(), var.begin(), toupper);

	if (var == "PIXEL_SAMPLE_CNT_EVAL") {
		sscanf_s(value, "%d", &PIXEL_SAMPLE_CNT_EVAL, sizeof(UINT32));
		CLAMP(PIXEL_SAMPLE_CNT_EVAL, 1, 2048);
	}
	else if (var == "PIXEL_SAMPLE_CNT_MORE") {
		sscanf_s(value, "%d", &PIXEL_SAMPLE_CNT_MORE, sizeof(UINT32));
		CLAMP(PIXEL_SAMPLE_CNT_MORE, 0, 2048);
	}
	else if (var == "PIXEL_SAMPLE_CNT_EDGE") {
		sscanf_s(value, "%d", &PIXEL_SAMPLE_CNT_EDGE, sizeof(UINT32));
		CLAMP(PIXEL_SAMPLE_CNT_EDGE, 0, 2048);
	}
	else if (var == "AREA_LIGHT_SAMP_CNT") {
		sscanf_s(value, "%d", &AREA_LIGHT_SAMP_CNT, sizeof(UINT32));
	}
	else if (var == "ENABLE_DOF") {
		sscanf_s(value, "%d", &ENABLE_DOF, sizeof(UINT32));
	}
	else if (var == "ENABLE_MB") {
		sscanf_s(value, "%d", &ENABLE_MB, sizeof(UINT32));
	}
	else
		return false;

	return true;
}



unsigned long GetConfigedThreadCount()
{
	if (0 == CPU_COUNT) {
		return GetCPUCount();
	}
	else
		return CPU_COUNT;
}
