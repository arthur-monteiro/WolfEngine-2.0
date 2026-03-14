#include "MultiThreadTaskManager.h"

#include <string>

#include <Debug.h>

#include "DynamicResourceUniqueOwnerArray.h"
#include "ProfilerCommon.h"

Wolf::MultiThreadTaskManager::ThreadGroupId Wolf::MultiThreadTaskManager::createThreadGroup(uint32_t threadCount, const std::string& name)
{
	m_threadGroups[m_threadGroupCount].initialize(threadCount, name, [this]() { return requestThreadInPool(); });
	m_threadGroupCount++;
	return m_threadGroupCount - 1;
}

void Wolf::MultiThreadTaskManager::addJobToThreadGroup(ThreadGroupId threadGroupId, const Job& job)
{
	m_threadGroups[threadGroupId].addJob(job);
}

void Wolf::MultiThreadTaskManager::executeJobsForThreadGroup(ThreadGroupId threadGroupId)
{
	PROFILE_FUNCTION

	ThreadGroup& threadGroup = m_threadGroups[threadGroupId];
	threadGroup.executeJobs();
}

void Wolf::MultiThreadTaskManager::waitForThreadGroup(ThreadGroupId threadGroupId)
{
	PROFILE_FUNCTION

	ThreadGroup& threadGroup = m_threadGroups[threadGroupId];
	threadGroup.waitJobsCompleted();
}

Wolf::MultiThreadTaskManager::Thread* Wolf::MultiThreadTaskManager::requestThreadInPool()
{
	m_requestedThreadCount++;
	return &m_threadPool[m_requestedThreadCount - 1];
}

#ifdef _WIN32
#include <windows.h>
#endif
void Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::execution(const std::string& name)
{
#ifdef _WIN32
	std::wstring wName = std::wstring(name.begin(), name.end());
	LPCWSTR swName = wName.c_str();
	SetThreadDescription(GetCurrentThread(), swName);
#endif 

	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_thread->mutex);
		Job jobToExecute;
		{
			PROFILE_SCOPED("Thread execution wait")

			m_thread->runCondition.wait(lock, [&] { return m_jobsPool->getNextJob(jobToExecute) || m_stopThreadRequested; });
		}

		if (m_stopThreadRequested)
		{
			break;
		}

		PROFILE_SCOPED("Thread jobs execution")

		do
		{
			jobToExecute();
		} while (m_jobsPool->getNextJob(jobToExecute));
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::notifyThreads() const
{
	m_thread->runCondition.notify_all();
}

void Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::waitJobsCompleted()
{
	m_thread->mutex.lock();
	m_thread->mutex.unlock();
}

void Wolf::MultiThreadTaskManager::ThreadGroup::initialize(uint32_t threadCount, const std::string& name, const std::function<Thread*()>& requestThreadInPool)
{
	m_jobsPool.reset(new JobsPool);

	for (uint32_t threadIdx = 0; threadIdx < threadCount; ++threadIdx)
	{
		m_threads.emplace_back(new PerThread(requestThreadInPool(), m_jobsPool.createNonOwnerResource(), name + std::to_string(threadIdx)));
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::addJob(const Job& job)
{
	m_jobsPool->addJob(job);
}

void Wolf::MultiThreadTaskManager::ThreadGroup::executeJobs()
{
	m_jobsPool->moveToNextFrame();

	Job jobToExecute;
	if (!m_jobsPool->getNextJob(jobToExecute))
		return;

	for (uint32_t threadIdx = 0; threadIdx < m_threads.size(); ++threadIdx)
	{
		ResourceUniqueOwner<PerThread>& thread = m_threads[threadIdx];
		thread->notifyThreads();
	}

	do
	{
		jobToExecute();
	} while (m_jobsPool->getNextJob(jobToExecute));
}

void Wolf::MultiThreadTaskManager::ThreadGroup::waitJobsCompleted()
{
	for (uint32_t threadIdx = 0; threadIdx < m_threads.size(); ++threadIdx)
	{
		ResourceUniqueOwner<PerThread>& thread = m_threads[threadIdx];
		thread->waitJobsCompleted();
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::JobsPool::addJob(const Job& job)
{
	m_nextJobs.emplace(job);
}

void Wolf::MultiThreadTaskManager::ThreadGroup::JobsPool::moveToNextFrame()
{
	m_currentJobs.swap(m_nextJobs);
}

bool Wolf::MultiThreadTaskManager::ThreadGroup::JobsPool::getNextJob(Job& outJob)
{
	m_currentJobsMutex.lock();

	if (m_currentJobs.empty())
	{
		m_currentJobsMutex.unlock();
		return false;
	}

	outJob = m_currentJobs.front();
	m_currentJobs.pop();

	m_currentJobsMutex.unlock();

	return true;
}

Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::PerThread(Thread* thread, const ResourceNonOwner<JobsPool>& jobsPool, const std::string& name) : m_thread(thread), m_jobsPool(jobsPool)
{
	m_thread->thread = std::thread(&PerThread::execution, this, name);
}

Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::~PerThread()
{
	{
		std::lock_guard<std::mutex> lk(m_thread->mutex);
		m_stopThreadRequested = true;
	}
	m_thread->runCondition.notify_all();
	m_thread->thread.join();
}