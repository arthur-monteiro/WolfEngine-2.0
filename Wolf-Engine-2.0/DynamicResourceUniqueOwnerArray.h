#pragma once

#include "DynamicStableArray.h"
#include "ResourceUniqueOwner.h"

namespace Wolf
{
#ifndef RESOURCE_TRACKING
	template <class T, size_t BatchSize = 8>
	using DynamicResourceUniqueOwnerArray = std::vector<ResourceUniqueOwner<T>>;
#else
	template <class T, size_t BatchSize = 8>
	using DynamicResourceUniqueOwnerArray = DynamicStableArray<ResourceUniqueOwner<T>, BatchSize>;
#endif

#ifndef RESOURCE_TRACKING
#define DYNAMIC_RESOURCE_UNIQUE_OWNER_ARRAY_RANGE_LOOP(arrayName, elementName, function) \
	for (auto& (elementName) : (arrayName)) \
	{ \
		function \
	}

#define DYNAMIC_RESOURCE_UNIQUE_OWNER_ARRAY_CONST_RANGE_LOOP(arrayName, elementName, function) \
	for (const auto& (elementName) : (arrayName)) \
	{ \
		function \
	}
#else
#define DYNAMIC_RESOURCE_UNIQUE_OWNER_ARRAY_RANGE_LOOP(arrayName, elementName, function) \
	for (uint32_t i = 0; i < (arrayName).size(); ++i) \
	{ \
		auto& (elementName) = (arrayName)[i]; \
		function \
	}

#define DYNAMIC_RESOURCE_UNIQUE_OWNER_ARRAY_CONST_RANGE_LOOP(arrayName, elementName, function) \
	for (uint32_t i = 0; i < (arrayName).size(); ++i) \
	{ \
		const auto& (elementName) = (arrayName)[i]; \
		function \
	}
#endif
}