#pragma once

#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "DynamicStableArray.h"

namespace Wolf
{
	class MultiThreadTaskManager
	{
	public:
		using ThreadGroupId = uint32_t;
		ThreadGroupId createThreadGroup(uint32_t threadCount, const std::string& name);

		using Job = std::function<void()>;
		void addJobToThreadGroup(ThreadGroupId threadGroupId, const Job& job);

		void executeJobsForThreadGroup(ThreadGroupId threadGroupId);
		void waitForThreadGroup(ThreadGroupId threadGroupId);

	private:
		struct Thread
		{
			std::thread thread;
			std::mutex mutex;
			std::condition_variable runCondition;
		};
		Thread* requestThreadInPool();
		static constexpr uint32_t MAX_THREAD_COUNT = 20;
		std::array<Thread, MAX_THREAD_COUNT> m_threadPool;
		uint32_t m_requestedThreadCount = 0;

		class ThreadGroup
		{
		public:
			ThreadGroup() = default;
			void initialize(uint32_t threadCount, const std::string& name, const std::function<Thread*()>& requestThreadInPool);

			void addJob(const Job& job);
			void executeJobs();
			void waitJobsCompleted();

		private:
			class JobsPool
			{
			public:
				void addJob(const Job& job);
				void moveToNextFrame();

				bool getNextJob(Job& outJob);

			private:
				std::queue<Job> m_nextJobs;

				std::mutex m_currentJobsMutex;
				std::queue<Job> m_currentJobs;
			};
			ResourceUniqueOwner<JobsPool> m_jobsPool;

			class PerThread
			{
			public:
				PerThread(Thread* thread, const ResourceNonOwner<JobsPool>& jobsPool, const std::string& name);
				~PerThread();

				void execution(const std::string& name);

				void notifyThreads() const;
				void waitJobsCompleted();

			private:
				Thread* m_thread;
				ResourceNonOwner<JobsPool> m_jobsPool;

				bool m_stopThreadRequested = false;
			};
			DynamicStableArray<ResourceUniqueOwner<PerThread>, 2> m_threads;
		};
		static constexpr uint32_t MAX_THREAD_GROUP_COUNT = 4;
		std::array<ThreadGroup, MAX_THREAD_GROUP_COUNT> m_threadGroups;
		uint32_t m_threadGroupCount = 0;
	};

}
