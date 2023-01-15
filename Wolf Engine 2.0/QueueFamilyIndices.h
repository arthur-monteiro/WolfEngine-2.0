#pragma once

namespace Wolf
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;
		int computeFamily = -1;

		bool isComplete()
		{
			return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
		}
	};
}