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

std::unique_ptr<UniquePass> pass;

void ChangeColor(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args)
{
	glm::vec3 color(args[0].ToInteger() / 255.0f, args[1].ToInteger() / 255.0f, args[2].ToInteger() / 255.0f);
	pass->setTriangleColor(color);
}

int main()
{
	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.configFilename = "config/config.ini";
	wolfInstanceCreateInfo.debugCallback = debugCallback;

	Wolf::TextFileReader UIHtmlReader("UI/UI.html");
	wolfInstanceCreateInfo.htmlStringUI = UIHtmlReader.getFileContent().c_str();

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	ultralight::JSObject jsObject;
	wolfInstance.getUserInterfaceJSObject(jsObject);
	jsObject["ChangeColor"] = std::function<void(const ultralight::JSObject&, const ultralight::JSArgs&)>(&ChangeColor);

	pass.reset(new UniquePass());
	wolfInstance.initializePass(pass.get());

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::PassBase*> passes(1);
		passes[0] = pass.get();

		wolfInstance.frame(passes, pass->getSemaphore());
	}

	wolfInstance.waitIdle();

	pass.release();

	return 0;
}