#pragma once

#include <fstream>

#include "GPUMemoryAllocatorInterface.h"

namespace Wolf
{
	class GPUMemoryDebug
	{
	public:
	    static void registerNewResource(GPUMemoryAllocatorInterface* gpuMemoryAllocatorInterface)
	    {
	        if (gpuMemoryAllocatorInterface)
	        {
	        	ms_resourcesMutex.lock();

	            ms_resources.push_back(gpuMemoryAllocatorInterface);
	            ms_totalMemoryAllocated += gpuMemoryAllocatorInterface->getMemoryAllocatedSize();
	            ms_totalMemoryRequested += gpuMemoryAllocatorInterface->getMemoryRequestedSize();

	        	ms_resourcesMutex.unlock();
	        }
	    }

	    static void unregisterResource(GPUMemoryAllocatorInterface* gpuMemoryAllocatorInterface)
	    {
	    	ms_resourcesMutex.lock();

	        auto it = std::find(ms_resources.begin(), ms_resources.end(), gpuMemoryAllocatorInterface);
	        if (it != ms_resources.end())
	        {
	            ms_totalMemoryAllocated -= gpuMemoryAllocatorInterface->getMemoryAllocatedSize();
	            ms_totalMemoryRequested -= gpuMemoryAllocatorInterface->getMemoryRequestedSize();
	            ms_resources.erase(it);
	        }
	        else
	        {
	            Debug::sendError("Unregistering resource that has not been registered");
	        }

	    	ms_resourcesMutex.unlock();
	    }

	    static void dumpJSON(const std::string& outFilename)
	    {
	    	ms_resourcesMutex.lock();

	        std::ofstream file(outFilename);
	        if (!file.is_open())
	        {
	            Debug::sendError("Cannot open file for writing: " + outFilename);
	            return;
	        }

	        file << "{\n";
	    	file << "  \"type\":\"GPU\",\n";
	        file << "  \"totalAllocated\": " << ms_totalMemoryAllocated << ",\n";
	        file << "  \"totalRequested\": " << ms_totalMemoryRequested << ",\n";
	        file << "  \"resources\": [\n";

	        for (size_t i = 0; i < ms_resources.size(); ++i)
	        {
	            auto* res = ms_resources[i];
	            bool isLast = (i == ms_resources.size() - 1);

	            file << "    {\n";
	            file << "      \"name\": \"" << escapeBackslashes(res->getName()) << "\",\n";
	            file << "      \"type\": \"" << (res->getType() == GPUMemoryAllocatorInterface::Type::BUFFER ? "Buffer" : "Image") << "\",\n";
	            file << "      \"allocatedBytes\": " << res->getMemoryAllocatedSize() << ",\n";
	            file << "      \"requestedBytes\": " << res->getMemoryRequestedSize() << ",\n";
	            file << "      \"isPoolOrAtlas\": " << (res->isPoolOrAtlas() ? "true" : "false") << ",\n";
	            file << "      \"usagePercent\": " << res->getUsagePercentage() << "\n";
	            file << "    }" << (isLast ? "" : ",") << "\n";
	        }

	        file << "  ]\n";
	        file << "}";

	        file.close();

	    	ms_resourcesMutex.unlock();
	    }

	    static uint64_t getTotalMemoryAllocated() { return ms_totalMemoryAllocated; }
	    static uint64_t getTotalMemoryRequested() { return ms_totalMemoryRequested; }

	private:
	    static std::string escapeBackslashes(const std::string& input)
	    {
	        std::string result;
	        result.reserve(input.size());

	        for (const char c : input)
	        {
	            if (c == '\\')
	            {
	                result += "\\\\";
	            }
	            else
	            {
	                result += c;
	            }
	        }

	        return result;
	    }

	    inline static uint64_t ms_totalMemoryAllocated = 0;
	    inline static uint64_t ms_totalMemoryRequested = 0;
	    inline static std::vector<GPUMemoryAllocatorInterface*> ms_resources;
		inline static std::mutex ms_resourcesMutex;
	};
}
