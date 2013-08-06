#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <common/defines/typedefs.h>


extern UINT32 PIXEL_SAMPLE_CNT_EVAL;
extern UINT32 PIXEL_SAMPLE_CNT_MORE;
extern UINT32 PIXEL_SAMPLE_CNT_EDGE;

extern UINT32 RT_THREAD_STACK_SIZE;
extern UINT32 LEAF_TRIANGLE_CNT;
extern UINT32 MAX_KD_DEPTH;
extern float  COLOR_DIFF_THRESH_HOLD;
extern UINT32 MAX_REFLECTION_BOUNCE;
extern UINT32 USE_TEX_MAP;
extern UINT32 ENABLE_DOF;
extern UINT32 ENABLE_MB;


// Comment out this to enable safe ray-triangle intersection algorithm
//#define USE_PACKED_PRIMITIVE



// Common constants
#define SQRT_TWO 1.4142135623730950488016887242097f


// Render options and global constants
bool SetGlobalConstant(const char* name, const char* value);
bool SetRenderOptions(const char* name, const char* value);
unsigned long GetConfigedThreadCount();
