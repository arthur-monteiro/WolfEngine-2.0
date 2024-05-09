#pragma once

namespace Wolf
{
	class Fence
	{
	public:
		static Fence* createFence(bool startSignaled);

		virtual ~Fence() = default;

		virtual void waitForFence() const = 0;
		virtual void resetFence() const = 0;
	};
}
