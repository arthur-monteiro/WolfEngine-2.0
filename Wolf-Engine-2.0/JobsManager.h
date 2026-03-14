#pragma once

#include "MultiThreadTaskManager.h"

namespace Wolf
{
    class JobsManager
    {
    public:
        JobsManager(uint32_t threadCountBeforeFrameAndRecord);
        ~JobsManager();

        void addJobBeforeFrame(const MultiThreadTaskManager::Job& job);
        void executeJobsBeforeFrame();

        enum class AddedJobStatus { SUCCESS, REJECTED };
        AddedJobStatus addStreamingJob(const MultiThreadTaskManager::Job& job);

    private:
        ResourceUniqueOwner<MultiThreadTaskManager> m_multiThreadTaskManager;
        MultiThreadTaskManager::ThreadGroupId m_beforeFrameAndRecordThreadGroupId;

        // Streaming
        void streamingExecution();

        std::queue<MultiThreadTaskManager::Job> m_streamingJobs;
        std::thread m_streamingThread;
        std::mutex m_streamingMutex;
        std::condition_variable m_streamingRunCondition;
        bool m_stopStreamingThreadRequested = false;
    };
}
