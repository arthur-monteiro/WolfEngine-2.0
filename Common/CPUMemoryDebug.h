#pragma once

#include <fstream>
#include <mutex>
#include <vector>

#include "Debug.h"

namespace Wolf
{
	class CPUMemoryDebug
	{
	public:
	    static void registerNewResource(const void* allocatorPtr, uint32_t size, const std::string& name)
	    {
	    	ms_resourcesMutex.lock();

            ms_resources.push_back({ allocatorPtr, size, name });
            ms_totalMemoryAllocated += size;

	    	ms_resourcesMutex.unlock();
	    }

		static void changeName(const void* allocatorPtr, const std::string& name)
	    {
	    	for (uint32_t allocatorIdx = 0; allocatorIdx < ms_resources.size(); ++allocatorIdx)
	    	{
	    		if (ms_resources[allocatorIdx].m_allocatorPtr == allocatorPtr)
	    		{
	    			ms_resources[allocatorIdx].m_name = name;
	    			return;
	    		}
	    	}

	    	Debug::sendCriticalError("Unregistering resource that has not been registered");
	    }

	    static void unregisterResource(const void* allocatorPtr)
	    {
	    	ms_resourcesMutex.lock();

	    	uint32_t allocatorIdx = 0;
	    	for (; allocatorIdx < ms_resources.size(); ++allocatorIdx)
	    	{
	    		if (ms_resources[allocatorIdx].m_allocatorPtr == allocatorPtr)
	    		{
	    			break;
	    		}
	    	}
	        if (allocatorIdx != ms_resources.size())
	        {
	            ms_totalMemoryAllocated -= ms_resources[allocatorIdx].m_size;
	            ms_resources.erase(ms_resources.begin() + allocatorIdx);
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
	    	file << "  \"type\":\"CPU\",\n";
	        file << "  \"totalAllocated\": " << ms_totalMemoryAllocated << ",\n";
	        file << "  \"resources\": [\n";

	        for (size_t i = 0; i < ms_resources.size(); ++i)
	        {
	            CPUMemoryAllocationInfo& res = ms_resources[i];
	            bool isLast = (i == ms_resources.size() - 1);

	            file << "    {\n";
	            file << "      \"name\": \"" << escapeBackslashes(res.m_name) << "\",\n";
	            file << "      \"allocatedBytes\": " << res.m_size << ",\n";
	            file << "      \"isPoolOrAtlas\": " << "false" << ",\n";
	            file << "      \"usagePercent\": " << "100" << "\n";
	            file << "    }" << (isLast ? "" : ",") << "\n";
	        }

	        file << "  ]\n";
	        file << "}";

	        file.close();

	    	ms_resourcesMutex.unlock();
	    }

	    static uint64_t getTotalMemoryAllocated() { return ms_totalMemoryAllocated; }

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
		struct CPUMemoryAllocationInfo
		{
			const void* m_allocatorPtr;
			uint32_t m_size;
			std::string m_name;
		};
	    inline static std::vector<CPUMemoryAllocationInfo> ms_resources;
		inline static std::mutex ms_resourcesMutex;
	};
}
