#include <WolfEngine.h>

#include "UniquePass.h"

void debugCallback(Wolf::Debug::Severity severity, Wolf::Debug::Type type, std::string message)
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

int main()
{
	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.configFilename = "config/config.ini";
	wolfInstanceCreateInfo.debugCallback = debugCallback;
	wolfInstanceCreateInfo.applicationName = "Variable Rate Shading";

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	UniquePass pass;
	wolfInstance.initializePass(&pass);

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::CommandRecordBase*> passes(1);
		passes[0] = &pass;

		wolfInstance.updateEvents();
		wolfInstance.frame(passes, pass.getSemaphore());
	}

	wolfInstance.waitIdle();

	return 0;
}