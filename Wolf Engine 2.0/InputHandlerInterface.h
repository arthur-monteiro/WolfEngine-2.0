#pragma once

namespace Wolf
{
	class InputHandlerInterface
	{
	public:
#ifndef __ANDROID__
		virtual void initialize(GLFWwindow* window) = 0;
#else
		virtual void initialize() = 0;
#endif
	};
}
