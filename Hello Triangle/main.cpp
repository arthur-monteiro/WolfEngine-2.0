#include <iostream>

#include <WolfEngine.h>

#include "UniquePass.h"

void debugCallback(Wolf::Debug::Severity severity, Wolf::Debug::Type type, const std::string& message)
{
	if (severity == Wolf::Debug::Severity::VERBOSE)
		return;
	
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
	}

	std::cout << message << std::endl;
}

int main(int argc, char* argv[])
{

	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.configFilename = "config/config.ini";
	wolfInstanceCreateInfo.debugCallback = debugCallback;
	wolfInstanceCreateInfo.applicationName = "Hello Triangle";

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	Wolf::ResourceUniqueOwner<UniquePass> pass(new UniquePass);
	wolfInstance.initializePass(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::ResourceNonOwner<Wolf::CommandRecordBase>> passes;
		passes.push_back(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

		wolfInstance.updateBeforeFrame();
		wolfInstance.frame(passes, pass->getSemaphore());
	}

	wolfInstance.waitIdle();

	return EXIT_SUCCESS;
}