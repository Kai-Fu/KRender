#include "thread_model.h"
#include "HelperFunc.h"
#include <assert.h>

KEvent::KEvent()
{
	pthread_mutex_init(&mMutex, NULL);
	pthread_cond_init(&mCond, NULL);

	mFlag = 0;
}

KEvent::~KEvent()
{
	pthread_mutex_destroy(&mMutex);
	pthread_cond_destroy(&mCond);
}

int KEvent::Wait()
{
	int ret = 0;
	pthread_mutex_lock(&mMutex);
	if (mFlag == 0)
		ret = pthread_cond_wait(&mCond, &mMutex);

	--mFlag;
	pthread_mutex_unlock(&mMutex);
	return ret;
}

int KEvent::Signal()
{
	pthread_mutex_lock(&mMutex);
	++mFlag;
	int ret = pthread_cond_signal(&mCond);
	pthread_mutex_unlock(&mMutex);
	return ret;
}


namespace ThreadModel {

void* ThreadBucket::TaskFuncion(void* lpParam)
{
	TaskParam* pParam = (TaskParam*)lpParam;

	ThreadBucket* pThreadBucket = (ThreadBucket*)pParam->pThreadBucket;
	while (1) {
		pThreadBucket->mThreadStartMutex[pParam->task_idx].Wait();

		if (pParam->pTask) {
			pParam->pTask->Execute();
			pParam->pTask = NULL;
		}
		else if (pThreadBucket->mbInDestory) {
			pthread_exit((void*)1);
		}

		pParam->pThreadBucket->NotifyTaskDone(pParam->task_idx);
	}

	pthread_exit((void*)0);
	return (void*)0;
}

ThreadBase::ThreadBase(UINT32 thread_cnt, PFnThreadRoutine pFunc)
{
	if (thread_cnt == INVALID_INDEX)
		thread_cnt = GetCPUCount();
	mTasks.resize(thread_cnt);
	mpThreadRountine = pFunc;
}

bool ThreadBase::InitThreads()
{
	// Create joinable threads
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (UINT32 i = 0; i < (UINT32)mTasks.size(); ++i) {
		mTasks[i].pThreadBucket = this;
		mTasks[i].task_idx = i;
		mTasks[i].pTask = NULL;

		int ret = pthread_create(&mTasks[i].pThreadHandle, &attr, mpThreadRountine, &mTasks[i]);

		if (ret) {
			// Something wrong when creating thread
			assert(0);
			return false;
		}
	}

	// Release the thread arribute object
	int ret = pthread_attr_destroy(&attr);
	assert(ret == 0);
	return true;
}

ThreadBase::~ThreadBase()
{

}

void ThreadBase::Destory()
{
	void *status;
	for (UINT32 i = 0; i < (UINT32)mTasks.size(); ++i) {
		int ret = pthread_join(mTasks[i].pThreadHandle, &status);
		assert(ret == 0);
	}
}

UINT32 ThreadBase::GetThreadCnt()
{
	return (UINT32)mTasks.size();
}

void ThreadBase::SetThreadTask(UINT32 idx, IThreadTask* pTask)
{
	mTasks[idx].pTask = pTask;
}

ThreadBucket::ThreadBucket() :
	ThreadBase(INVALID_INDEX, TaskFuncion)
{
	mThreadStartMutex = NULL;
	Init((UINT32)mTasks.size());
}

ThreadBucket::ThreadBucket(UINT32 thread_cnt) :
	ThreadBase(thread_cnt, TaskFuncion)
{
	mThreadStartMutex = NULL;
	Init((UINT32)mTasks.size());
}

void ThreadBucket::Init(UINT32 thread_cnt)
{
	mbInDestory = false;
	mSpinLock = 0;
	mThreadCntInProgress = 0;

	mThreadStartMutex = new KEvent[thread_cnt];

	InitThreads();
}

ThreadBucket::~ThreadBucket()
{
	mbInDestory = true;
	for (size_t i = 0; i < mTasks.size(); ++i) {
		mTasks[i].pTask = NULL;
		// Unlock the mutex so that the threads teminates normally.
		int ret = mThreadStartMutex[i].Signal();
		assert(ret == 0);
	}
	Destory();
	delete[] mThreadStartMutex;
}

void ThreadBucket::NotifyTaskDone(UINT32 idx)
{
	EnterSpinLockCriticalSection(mSpinLock);
	atomic_decrement(&mThreadCntInProgress);
	if (0 == mThreadCntInProgress) {
		int ret = mFinishEvent.Signal();
		assert(ret == 0);
	}
	LeaveSpinLockCriticalSection(mSpinLock);
}

bool ThreadBucket::Run()
{
	assert(mThreadCntInProgress == 0);
	if (mTasks.size() == 1)
		mTasks[0].pTask->Execute();
	else {
		mThreadCntInProgress = (long)mTasks.size();

		for (size_t i = 0; i < mTasks.size(); ++i) {
			int ret = mThreadStartMutex[i].Signal();
			assert(ret == 0);
		}

		mFinishEvent.Wait();

	}
	return true;
}


void ThreadPool::Init(UINT32 thread_cnt)
{
	mIdleAccessCS = 0;
	mbInDestory = false;
	mIdleThreadIdx = thread_cnt;
	mIdleThreads.resize(thread_cnt);
	mThreadStartMutex = new KEvent[thread_cnt];
	for (UINT32 i = 0; i < thread_cnt; ++i)
		mIdleThreads[i] = i;

	mWaitIssued = 0;
	InitThreads();
}

ThreadPool::ThreadPool() :
	ThreadBase(INVALID_INDEX, TaskFuncion)
{
	mThreadStartMutex = NULL;
	Init((UINT32)mTasks.size());
}

ThreadPool::ThreadPool(UINT32 thread_cnt) :
	ThreadBase(thread_cnt, TaskFuncion)
{
	mThreadStartMutex = NULL;
	Init((UINT32)mTasks.size());
}

ThreadPool::~ThreadPool()
{
	mbInDestory = true;
	assert(mIdleThreadIdx == (long)mTasks.size());
	for (size_t i = 0; i < mTasks.size(); ++i) {
		mTasks[i].pTask = NULL;
		mThreadStartMutex[i].Signal();
	}
	Destory();
	delete[] mThreadStartMutex;
}


void ThreadPool::NotifyTaskDone(UINT32 idx)
{
	EnterSpinLockCriticalSection(mIdleAccessCS);
	mIdleThreads[mIdleThreadIdx++] = idx;
#ifdef DEBUG
	for (long i = 0; i < mIdleThreadIdx - 1; ++i) {
		assert(mIdleThreads[i] != idx);
	}
	assert(mBusyThreads.find(idx) == mBusyThreads.end());
#endif
	mBusyThreads.erase(idx);
	if (mWaitIssued && mIdleThreadIdx == (long)mTasks.size()) {
		mIdleFlag.Signal();
	}
	LeaveSpinLockCriticalSection(mIdleAccessCS);
}

void* ThreadPool::TaskFuncion(void* lpParam)
{
	TaskParam* pParam = (TaskParam*)lpParam;
	ThreadPool* pThreadPool = (ThreadPool*)pParam->pThreadBucket;

	while (1) {
		pThreadPool->mThreadStartMutex[pParam->task_idx].Wait();
		if (pParam->pTask) {
			pParam->pTask->Execute();
		}
		else if (pThreadPool->mbInDestory) {
			pthread_exit((void*)1);
		}
		pThreadPool->NotifyTaskDone(pParam->task_idx);
	}

	pthread_exit((void*)0);
	return (void*)0;
}

UINT32 ThreadPool::PopIdleThreadIndex()
{
	EnterSpinLockCriticalSection(mIdleAccessCS);
	
	UINT32 ret = INVALID_INDEX;
	if (mIdleThreadIdx != 0) {
		--mIdleThreadIdx;
		ret = mIdleThreads[mIdleThreadIdx];
		mBusyThreads.insert(ret);
	}
	LeaveSpinLockCriticalSection(mIdleAccessCS);
	return ret;
}

void ThreadPool::KickIdleThread(UINT32 idx)
{
#ifdef DEBUG
	EnterSpinLockCriticalSection(mIdleAccessCS);
	for (long i = 0; i < mIdleThreadIdx; ++i) {
		assert(mIdleThreads[i] != idx);
	}
	LeaveSpinLockCriticalSection(mIdleAccessCS);
#endif

	mThreadStartMutex[idx].Signal();

}

void ThreadPool::KickScheduledThreads()
{
	EnterSpinLockCriticalSection(mIdleAccessCS);

	if (!mBusyThreads.empty()) {

		for (std::set<UINT32>::iterator it = mBusyThreads.begin(); it != mBusyThreads.end(); ++it) {
			mThreadStartMutex[*it].Signal();
		}
	}
	else
		mIdleFlag.Signal();

	LeaveSpinLockCriticalSection(mIdleAccessCS);
}

void ThreadPool::WaitForAll()
{
	atomic_increment(&mWaitIssued);
	if (mIdleThreadIdx < (long)mTasks.size())
		mIdleFlag.Wait();
	atomic_decrement(&mWaitIssued);
}

void ThreadPool::SetThreadTask(UINT32 idx, IThreadTask* pTask)
{
	mTasks[idx].pTask = pTask;
}

void ThreadPool::ReleaseScheduledThread(UINT32 idx)
{
	NotifyTaskDone(idx);
}

TaskQueue::TaskQueue()
{
	mIsIdle = 0;
}

TaskQueue::~TaskQueue()
{
}

bool TaskQueue::IsIdle()
{
	return (mIsIdle == 0);
}

UINT32 TaskQueue::AddTask(IThreadTask* pTask)
{
	mTaskList.push_back(pTask);
	return (UINT32)mTaskList.size() - 1;
}

bool TaskQueue::KickStart()
{
	if (!IsIdle())
		return false;

	atomic_increment(&mIsIdle);
	mCurrentTask = mTaskList.begin();

	// Create joinable threads
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&mExecutingThread, &attr, TaskFuncion, (void*)this);
	// Release the thread arribute object
	int ret = pthread_attr_destroy(&attr);
	assert(ret == 0);

	return true;
}

bool TaskQueue::WaitForAll()
{
	void *status;
	if (0 == pthread_join(mExecutingThread, &status))
		return true;
	else
		return false;
}

IThreadTask* TaskQueue::GetNextTask()
{
	IThreadTask* ret = NULL;
	if (mCurrentTask != mTaskList.end()) {
		ret = *mCurrentTask;
		++mCurrentTask;
	}

	return ret;
}

void TaskQueue::NotifyAllDone()
{
	mTaskList.clear();
	atomic_decrement(&mIsIdle);
}

void* TaskQueue::TaskFuncion(void* lpParam)
{
	TaskQueue* pTaskQueue = (TaskQueue*)lpParam;

	IThreadTask* pTask = NULL;
	while (pTask = pTaskQueue->GetNextTask()) {
		pTask->Execute();
	}

	pTaskQueue->NotifyAllDone();

	pthread_exit((void*)0);
	return (void*)0;
}


} // namespace