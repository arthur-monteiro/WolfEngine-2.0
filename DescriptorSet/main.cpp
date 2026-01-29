#include <WolfEngine.h>

#include "UniquePass.h"

void debugCallback(Wolf::Debug::Severity severity, Wolf::Debug::Type type, const std::string& message)
{
	switch (severity)
	{
	case Wolf::Debug::Severity::ERROR:
		std::cout << "Error : ";
		break;
	case Wolf::Debug::Severity::WARNING:
		std::cout << "Warning : ";
		break;
	case Wolf::Debug::Severity::INFO:
		std::cout << "Info : ";
		break;
	case Wolf::Debug::Severity::VERBOSE:
		return;
	}

	std::cout << message << std::endl;
}

int main(int argc, char* argv[])
{
	bool doScreenshot = false;
	bool exitAfterFirstFrame = false;
	if (argc >= 2)
	{
		const std::string option = argv[1];
		if (option == "--screenshotThenQuit")
		{
			doScreenshot = true;
			exitAfterFirstFrame = true;
		}
	}

	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.m_configFilename = "config/config.ini";
	wolfInstanceCreateInfo.m_debugCallback = debugCallback;

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	Wolf::ResourceUniqueOwner<UniquePass> pass(new UniquePass);
	wolfInstance.initializePass(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

	if (doScreenshot)
	{
		pass->requestScreenshot();
	}

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::ResourceNonOwner<Wolf::CommandRecordBase>> passes;
		passes.push_back(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

		wolfInstance.updateBeforeFrame();
		uint32_t swapChainImageIdx = wolfInstance.acquireNextSwapChainImage();
		wolfInstance.frame(passes, pass->getSemaphore(swapChainImageIdx), swapChainImageIdx);

		if (exitAfterFirstFrame)
		{
			break;
		}
	}

	wolfInstance.waitIdle();

	return 0;
}