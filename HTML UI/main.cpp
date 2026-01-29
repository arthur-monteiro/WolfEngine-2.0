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

Wolf::ResourceUniqueOwner<UniquePass> pass;

void setTriangleColor(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args)
{
	const glm::vec3 color(args[0].ToInteger() / 255.0f, args[1].ToInteger() / 255.0f, args[2].ToInteger() / 255.0f);
	pass->setTriangleColor(color);
}

int main()
{
	Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
	wolfInstanceCreateInfo.m_configFilename = "config/config.ini";
	wolfInstanceCreateInfo.m_debugCallback = debugCallback;
	wolfInstanceCreateInfo.m_htmlURL = "UI/UI.html";
	wolfInstanceCreateInfo.m_uiFinalLayout = VK_IMAGE_LAYOUT_GENERAL;

	Wolf::WolfEngine wolfInstance(wolfInstanceCreateInfo);

	pass.reset(new UniquePass);
	wolfInstance.initializePass(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

	ultralight::JSObject jsObject;
	wolfInstance.getUserInterfaceJSObject(jsObject);
	jsObject["ChangeColor"] = ultralight::JSCallback(&setTriangleColor);

	while (!wolfInstance.windowShouldClose())
	{
		std::vector<Wolf::ResourceNonOwner<Wolf::CommandRecordBase>> passes;
		passes.push_back(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

		wolfInstance.updateBeforeFrame();
		uint32_t swapChainImageIdx = wolfInstance.acquireNextSwapChainImage();
		wolfInstance.frame(passes, pass->getSemaphore(swapChainImageIdx), swapChainImageIdx);
	}

	wolfInstance.waitIdle();
	pass.reset(nullptr);

	return 0;
}