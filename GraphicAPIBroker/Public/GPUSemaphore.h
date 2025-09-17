#pragma once

#include "Enums.h"

namespace Wolf
{
    class Semaphore
    {
    public:
        static Semaphore* createSemaphore(uint32_t pipelineStage);

        virtual ~Semaphore() = default;
    };
}
