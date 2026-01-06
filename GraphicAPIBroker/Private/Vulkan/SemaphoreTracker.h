#pragma once

#include <cstdint>
#include <unordered_map>

namespace Wolf
{
    class Semaphore;

    class SemaphoreTracker
    {
    public:
        SemaphoreTracker();

        void trackSignal(const Semaphore* semaphore, uint64_t value)
        {
            m_submittedValues[semaphore] = value;
        }

        bool isValidWait(const Semaphore* semaphore, uint64_t waitValue)
        {
            uint64_t lastSignaled = 0;
            if (m_submittedValues.find(semaphore) != m_submittedValues.end())
            {
                lastSignaled = m_submittedValues[semaphore];
            }

            return lastSignaled >= waitValue;
        }

    private:
        std::unordered_map<const Semaphore*, uint64_t> m_submittedValues;
    };

    extern SemaphoreTracker* g_semaphoreTracker;
}
