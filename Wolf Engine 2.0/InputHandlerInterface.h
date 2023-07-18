#pragma once

namespace Wolf
{
	class InputHandlerInterface
	{
	public:
		virtual void initialize(GLFWwindow* window) = 0;
	};
}
