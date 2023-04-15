#pragma once

namespace Wolf
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;
		int computeFamily = -1;

		bool isComplete() const
		{
			return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
		}
	};
}