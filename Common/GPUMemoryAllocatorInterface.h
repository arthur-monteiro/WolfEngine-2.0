#pragma once

#include <cstdint>
#include <string>

namespace Wolf
{
    class GPUMemoryAllocatorInterface
    {
    public:
        virtual ~GPUMemoryAllocatorInterface() {}

        [[nodiscard]] virtual uint32_t getMemoryAllocatedSize() const = 0;
        [[nodiscard]] virtual uint32_t getMemoryRequestedSize() const = 0;

        enum class Type { BUFFER, IMAGE };
        [[nodiscard]] virtual Type getType() const = 0;

        enum class FLAG_VALUE : uint32_t { TRANSIENT = 1 };
        [[nodiscard]] virtual uint32_t getFlags() const { return 0; }

        [[nodiscard]] virtual std::string getName() const = 0;

        [[nodiscard]] virtual bool isPoolOrAtlas() const = 0;
        [[nodiscard]] virtual float getUsagePercentage() const { return 100.0f; };

    protected:
        GPUMemoryAllocatorInterface() = default;

    private:
    };
}
