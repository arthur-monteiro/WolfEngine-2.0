#include "CommandRecordBase.h"

void Wolf::CommandRecordBase::createSemaphores(const InitializationContext& context, uint32_t pipelineStage, bool isFinalPass)
{
    m_semaphores.resize(isFinalPass ? context.swapChainImageCount : 1);
    for (uint32_t i = 0; i < m_semaphores.size(); ++i)
    {
        m_semaphores[i].reset(Semaphore::createSemaphore(pipelineStage, isFinalPass ? Semaphore::Type::BINARY : Semaphore::Type::TIMELINE));
    }
}
