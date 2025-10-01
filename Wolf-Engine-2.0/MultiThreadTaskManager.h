#pragma once

#include <array>
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
			class PerThread
			{
			public:
				PerThread();
				~PerThread();
				PerThread(Thread* thread, const std::string& name);

				void addJob(const Job& job) { m_nextJobs.emplace(job); }
				void execution(const std::string& name);

				void executeJobs();
				void waitJobsCompleted();

			private:
				Thread* m_thread;

				std::queue<Job> m_nextJobs;
				std::queue<Job> m_currentJobs;

				bool m_stopThreadRequested = false;
			};
			DynamicStableArray<ResourceUniqueOwner<PerThread>, 2> m_threads;

			uint32_t m_lastThreadToReceiveWork = 0;
		};
		static constexpr uint32_t MAX_THREAD_GROUP_COUNT = 4;
		std::array<ThreadGroup, MAX_THREAD_GROUP_COUNT> m_threadGroups;
		uint32_t m_threadGroupCount = 0;
	};

}
