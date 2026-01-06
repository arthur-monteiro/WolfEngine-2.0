#include "SemaphoreTracker.h"

#include "Debug.h"

Wolf::SemaphoreTracker* Wolf::g_semaphoreTracker = nullptr;

Wolf::SemaphoreTracker::SemaphoreTracker()
{
    if (g_semaphoreTracker != nullptr)
    {
        Debug::sendCriticalError("Can't create multiple instances of semaphore tracker");
    }
    g_semaphoreTracker = this;
}

