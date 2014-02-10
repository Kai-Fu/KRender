#pragma once

#include "../base/base_header.h"
#include "../os/api_wrapper.h"
#include "helper_func.h"
#include <pthread.h>
#include <vector>
#include <set>
#include <list>


class KEvent
{
public:
	KEvent();
	~KEvent();

	int Wait();
	int Signal();

private:
	pthread_cond_t mCond;
	pthread_mutex_t mMutex;
	volatile long mFlag;
};

namespace ThreadModel {

	// Interface class that represents tasks
	class IThreadTask
	{
	public:
		virtual void Execute() = 0;
	};

	class ThreadBase;
	class ThreadBucket;

	struct TaskParam
	{
		ThreadBase* pThreadBucket;
		UINT32 task_idx;
		IThreadTask* pTask;
		pthread_t pThreadHandle;
	};
	typedef void* (*PFnThreadRoutine)(void* lpThreadParam);
	class ThreadBase
	{
	public:
		ThreadBase(UINT32 thread_cnt, PFnThreadRoutine pFunc);
		virtual ~ThreadBase();
		virtual void NotifyTaskDone(UINT32 idx) = 0;
		UINT32 GetThreadCnt();
		void SetThreadTask(UINT32 idx, IThreadTask* pTask);
	protected:
		bool InitThreads();
		void Destory();
	protected:
		PFnThreadRoutine mpThreadRountine;
		std::vector<TaskParam> mTasks;
		volatile bool mbInDestory;
	};
	// A thread bucket is a bunch of threads that are expected to take similiar tasks
	// and their time consumption will be almost the same. The running function is synchronized
	// in the calling thread.
	class ThreadBucket : public ThreadBase
	{
	public:
		ThreadBucket();
		ThreadBucket(UINT32 thread_cnt);
		virtual ~ThreadBucket();

		bool Run();
	private:
		void Init(UINT32 thread_cnt);
		virtual void NotifyTaskDone(UINT32 idx);
		static void* TaskFuncion(void* lpParam);
	protected:
		KEvent mFinishEvent;
		volatile long mThreadCntInProgress;
		KEvent* mThreadStartMutex;
		SPIN_LOCK_FLAG mSpinLock;
	};

	class ThreadPool : public ThreadBase
	{
	public:
		ThreadPool();
		ThreadPool(UINT32 thread_cnt);
		virtual ~ThreadPool();

		UINT32 PopIdleThreadIndex();
		void KickIdleThread(UINT32 idx);
		void KickScheduledThreads();
		void ReleaseScheduledThread(UINT32 idx);
		void WaitForAll();
		void SetThreadTask(UINT32 idx, IThreadTask* pTask);
	private:
		void Init(UINT32 thread_cnt);
		virtual void NotifyTaskDone(UINT32 idx);
		static void* TaskFuncion(void* lpParam);

	protected:
		long mIdleThreadIdx;
		SPIN_LOCK_FLAG mIdleAccessCS;
		KEvent* mThreadStartMutex;
		std::vector<UINT32> mIdleThreads;
		std::set<UINT32> mBusyThreads;
		KEvent mIdleFlag;
		volatile long mWaitIssued;;

	};

	class TaskQueue
	{
	public:
		TaskQueue();
		~TaskQueue();

		bool IsIdle();
		UINT32 AddTask(IThreadTask* pTask);
		bool KickStart();
		bool WaitForAll();

		IThreadTask* GetNextTask();
		void NotifyAllDone();
	private:
		static void* TaskFuncion(void* lpParam);

	protected:
		std::list<IThreadTask*> mTaskList;
		std::list<IThreadTask*>::iterator mCurrentTask;
		pthread_t mExecutingThread;
		volatile long mIsIdle;
	};
}
