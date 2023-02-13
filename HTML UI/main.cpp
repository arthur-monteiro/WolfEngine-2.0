#include <TextFileReader.h>
#include <WolfEngine.h>

#include "UniquePass.h"

void debugCallback(Wolf::Debug::Severity severity, Wolf::Debug::Type type, std::string message)
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

int main()
{
	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.configFilename = "config/config.ini";
	wolfInstanceCreateInfo.debugCallback = debugCallback;

	Wolf::TextFileReader UIHtmlReader("UI/UI.html");
	wolfInstanceCreateInfo.htmlStringUI = UIHtmlReader.getFileContent().c_str();

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	UniquePass pass;
	wolfInstance.initializePass(&pass);

	ultralight::JSObject jsObject;
	wolfInstance.getUserInterfaceJSObject(jsObject);
	jsObject["ChangeColor"] = std::bind(&UniquePass::setTriangleColor, &pass, std::placeholders::_1, std::placeholders::_2);

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::CommandRecordBase*> passes(1);
		passes[0] = &pass;

		wolfInstance.frame(passes, pass.getSemaphore());
	}

	wolfInstance.waitIdle();

	return 0;
}