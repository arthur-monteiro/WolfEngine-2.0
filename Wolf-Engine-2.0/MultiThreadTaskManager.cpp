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
		{
			PROFILE_SCOPED("Thread execution wait")

			m_thread->runCondition.wait(lock, [&] { return !m_nextJobs.empty() || m_stopThreadRequested; });
		}

		if (m_stopThreadRequested)
		{
			break;
		}

		PROFILE_SCOPED("Thread jobs execution")

		m_currentJobs.swap(m_nextJobs);
		if (!m_nextJobs.empty())
			Debug::sendError("There shouldn't be jobs left here");

		while (!m_currentJobs.empty())
		{
			Job& job = m_currentJobs.front();
			job();
			m_currentJobs.pop();
		}
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::executeJobs()
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
	for (uint32_t threadIdx = 0; threadIdx < threadCount; ++threadIdx)
	{
		m_threads.emplace_back(new PerThread(requestThreadInPool(), name + std::to_string(threadIdx)));
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::addJob(const Job& job)
{
	m_lastThreadToReceiveWork = (m_lastThreadToReceiveWork + 1) % m_threads.size();
	uint32_t threadIdx = m_lastThreadToReceiveWork;

	m_threads[threadIdx]->addJob(job);
}

void Wolf::MultiThreadTaskManager::ThreadGroup::executeJobs()
{
	for (uint32_t threadIdx = 0; threadIdx < m_threads.size(); ++threadIdx)
	{
		ResourceUniqueOwner<PerThread>& thread = m_threads[threadIdx];
		thread->executeJobs();
	}
}

void Wolf::MultiThreadTaskManager::ThreadGroup::waitJobsCompleted()
{
	for (uint32_t threadIdx = 0; threadIdx < m_threads.size(); ++threadIdx)
	{
		ResourceUniqueOwner<PerThread>& thread = m_threads[threadIdx];
		thread->waitJobsCompleted();
	}
}

Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::PerThread()
{
	m_thread = nullptr;
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

Wolf::MultiThreadTaskManager::ThreadGroup::PerThread::PerThread(Thread* thread, const std::string& name) : m_thread(thread)
{
	m_thread->thread = std::thread(&PerThread::execution, this, name);
}
