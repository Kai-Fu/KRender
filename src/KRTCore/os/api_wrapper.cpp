#include "api_wrapper.h"

#ifdef __GNUC__
	#include <stdio.h>
	#include <sys/time.h>
	#include <time.h>
	#include <unistd.h>
#else
	#include <windows.h>
	#include <intrin.h>
#endif


void EnterSpinLockCriticalSection(SPIN_LOCK_FLAG& spinLockFlag)
{
	unsigned int cnt = 0;
	while (1) {
		long orig = atomic_compare_exchange(&spinLockFlag, 1, 0);
		if (orig == 0 && spinLockFlag == 1)
			break;
		++cnt;
		if (cnt > 0x00000fff)
			SleepForMS(20);
	}
}

void EnterSpinLockCriticalSection_Share(SPIN_LOCK_FLAG& spinLockFlag)
{
	unsigned int cnt = 0;
	while (1) {
		long orig = atomic_compare_exchange(&spinLockFlag, 2, 0);
		if ((orig == 0 && spinLockFlag == 2) ||
			orig > 2)
			break;
		++cnt;
		if (cnt > 0x00000fff)
			SleepForMS(20);
	}
	atomic_increment(&spinLockFlag);
}

void LeaveSpinLockCriticalSection_Share(SPIN_LOCK_FLAG& spinLockFlag)
{
	atomic_decrement(&spinLockFlag);
	atomic_compare_exchange(&spinLockFlag, 0, 2);
}


void LeaveSpinLockCriticalSection(SPIN_LOCK_FLAG& spinLockFlag)
{
	spinLockFlag = 0;
}


void SleepForMS(UINT32 t)
{
#ifdef __GNUC__
	usleep(t * 1000);
#else
	Sleep(t);
#endif
}

double GetSystemElapsedTime()
{
#ifdef __GNUC__
	struct timeval v;
	gettimeofday(&v,0);
	return double(v.tv_sec) + double(v.tv_usec / 1000000.0);

#else
	LARGE_INTEGER t;
	LARGE_INTEGER freq;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&freq);
	return (double)(t.QuadPart) / (double)freq.QuadPart;
#endif
}

unsigned long GetCPUCount()
{
#ifdef __GNUC__
	long nProcessorsOnline = sysconf(_SC_NPROCESSORS_ONLN);
	return (unsigned long)nProcessorsOnline;
#else
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#endif
}

long atomic_compare_exchange(volatile long* value, long new_val, long old_val)
{
#ifdef __GNUC__
	return __sync_val_compare_and_swap(value, old_val, new_val);
#else
	return _InterlockedCompareExchange(value, new_val, old_val);
#endif
}

long atomic_increment(volatile long* value)
{
#ifdef __GNUC__
	return __sync_add_and_fetch(value, 1);	
#else
	return _InterlockedIncrement(value);
#endif
}

long atomic_decrement(volatile long* value)
{
#ifdef __GNUC__
	return __sync_sub_and_fetch(value, 1);	
#else
	return _InterlockedDecrement(value);
#endif
}

long atomic_exchange_add(volatile long* value, long param)
{
#ifdef __GNUC__
	return __sync_add_and_fetch(value, param);	
#else
	return _InterlockedExchangeAdd(value, param);
#endif
}

bool atomic_compare_exchange_ptr(void** value, void* new_val, void* old_val)
{

#ifdef __GNUC__
	#if __SIZEOF_POINTER__ == 8 
		return __sync_bool_compare_and_swap(
			reinterpret_cast<long long volatile *>(value), reinterpret_cast<long long>(old_val), 
			reinterpret_cast<long long>(new_val));
	#else
		return __sync_bool_compare_and_swap(
			reinterpret_cast<long volatile *>(value), reinterpret_cast<long>(old_val), 
			reinterpret_cast<long>(new_val));
	#endif
#else
	return InterlockedCompareExchangePointer(value, new_val, old_val) == old_val;
#endif
}

void* Aligned_Malloc(size_t size, size_t align)
{
#ifdef __GNUC__
	return posix_memalign(NULL, align, size);
#else
	return _aligned_malloc(size, align);
#endif
}

void Aligned_Free(void* ptr)
{
#ifdef __GNUC__
	return free(ptr);
#else
	return _aligned_free(ptr);
#endif

}

void* Aligned_Realloc(void* ptr, size_t size, size_t align)
{
#ifdef __GNUC__
	return posix_memalign(ptr, align, size);
#else
	return _aligned_realloc(ptr, size, align);
#endif
}
