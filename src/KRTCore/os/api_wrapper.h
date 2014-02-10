#pragma once

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#include "../base/base_header.h"
#include <vector>

typedef volatile long SPIN_LOCK_FLAG;
typedef volatile long LOCK_FREE_LONG;

void EnterSpinLockCriticalSection(SPIN_LOCK_FLAG& spinLockFlag);
void LeaveSpinLockCriticalSection(SPIN_LOCK_FLAG& spinLockFlag);
/* This pair of functions is typically used when one or more threads are performing inserting elements into a STL vector,
   which should use EnterSpinLockCriticalSection to guard the inserting operation. Meanwhile multiple threads are performing
   indexing or lookup operation, which should use EnterSpinLockCriticalSection_Share.
*/
void EnterSpinLockCriticalSection_Share(SPIN_LOCK_FLAG& spinLockFlag);
void LeaveSpinLockCriticalSection_Share(SPIN_LOCK_FLAG& spinLockFlag);

long atomic_compare_exchange(volatile long* value, long new_val, long old_val);
long atomic_increment(volatile long* value);
long atomic_decrement(volatile long* value);
long atomic_exchange_add(volatile long* value, long param);
bool atomic_compare_exchange_ptr(void** value, void* new_val, void* old_val);


unsigned long GetCPUCount();
void SleepForMS(UINT32 t);
double GetSystemElapsedTime();

void* Aligned_Malloc(size_t size, size_t align);
void Aligned_Free(void* ptr);
void* Aligned_Realloc(void* ptr, size_t size, size_t align);
// TODO: put thread related OS API here
