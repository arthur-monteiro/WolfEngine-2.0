#pragma once

#include "Enums.h"

namespace Wolf
{
    class Semaphore
    {
    public:
        enum class Type { BINARY, TIMELINE };
        static Semaphore* createSemaphore(uint32_t pipelineStage, Type type);

        virtual ~Semaphore() = default;

        [[nodiscard]] Type getType() const { return m_type; }

    protected:
        Type m_type;
    };
}
