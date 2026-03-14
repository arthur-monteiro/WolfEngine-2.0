#include "JobsManager.h"

#include "ProfilerCommon.h"

Wolf::JobsManager::JobsManager(uint32_t threadCountBeforeFrameAndRecord)
{
    m_multiThreadTaskManager.reset(new MultiThreadTaskManager);
    m_beforeFrameAndRecordThreadGroupId = m_multiThreadTaskManager->createThreadGroup(threadCountBeforeFrameAndRecord, "Before frame and record");

    m_streamingThread = std::thread(&JobsManager::streamingExecution, this);
}

Wolf::JobsManager::~JobsManager()
{
    {
        std::lock_guard<std::mutex> lk(m_streamingMutex);
        m_stopStreamingThreadRequested = true;
    }
    m_streamingRunCondition.notify_all();
    m_streamingThread.join();
}

void Wolf::JobsManager::addJobBeforeFrame(const MultiThreadTaskManager::Job& job)
{
    m_multiThreadTaskManager->addJobToThreadGroup(m_beforeFrameAndRecordThreadGroupId, job);
}

void Wolf::JobsManager::executeJobsBeforeFrame()
{
    m_multiThreadTaskManager->executeJobsForThreadGroup(m_beforeFrameAndRecordThreadGroupId);
    m_multiThreadTaskManager->waitForThreadGroup(m_beforeFrameAndRecordThreadGroupId);
}

Wolf::JobsManager::AddedJobStatus Wolf::JobsManager::addStreamingJob(const MultiThreadTaskManager::Job& job)
{
    m_streamingMutex.lock();
    bool needNotify = m_streamingJobs.empty();
    m_streamingJobs.emplace(job);
    m_streamingMutex.unlock();

    if (needNotify)
    {
        m_streamingRunCondition.notify_all();
    }

    return AddedJobStatus::SUCCESS;
}

#ifdef _WIN32
#include <windows.h>
#endif
void Wolf::JobsManager::streamingExecution()
{
    std::string threadName = "Streaming thread";

#ifdef _WIN32
    std::wstring wName = std::wstring(threadName.begin(), threadName.end());
    LPCWSTR swName = wName.c_str();
    SetThreadDescription(GetCurrentThread(), swName);
#endif

    for (;;)
    {
        MultiThreadTaskManager::Job job;

        {
            PROFILE_SCOPED("Streaming thread execution wait")

            std::unique_lock<std::mutex> lock(m_streamingMutex);

            m_streamingRunCondition.wait(lock, [&]
            {
                return !m_streamingJobs.empty() || m_stopStreamingThreadRequested;
            });

            if (m_stopStreamingThreadRequested)
            {
                break;
            }

            job = std::move(m_streamingJobs.front());
            m_streamingJobs.pop();
        }

        PROFILE_SCOPED("Streaming jobs execution")

        job();

        for (;;)
        {
            {
                std::lock_guard<std::mutex> lock(m_streamingMutex);
                if (m_streamingJobs.empty())
                {
                    break;
                }
                job = std::move(m_streamingJobs.front());
                m_streamingJobs.pop();
            }

            job();
        }
    }
}
