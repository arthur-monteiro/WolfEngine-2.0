#include "Configuration.h"

#include <fstream>
#include <sstream>

#include "Debug.h"

const Wolf::Configuration* Wolf::g_configuration = nullptr;

#ifndef __ANDROID__
Wolf::Configuration::Configuration(const std::string& filename)
#else
Wolf::Configuration::Configuration(const std::string& filename, AAssetManager* androidAssetManager) : m_androidAssetManager(androidAssetManager)
#endif
{
	if (g_configuration)
		Debug::sendCriticalError("Can't instantiate Configuration twice");

	std::ifstream configFile(filename);
	std::string line;
	while (std::getline(configFile, line))
	{
		if (const size_t pos = line.find("="); pos != std::string::npos)
		{
			std::string token = line.substr(0, pos);
			line.erase(0, pos + 1);

			if (token == "windowWidth")
				m_windowWidth = std::stoi(line);
			if (token == "windowHeight")
				m_windowHeight = std::stoi(line);
			if (token == "maxCachedFrames")
				m_maxCachedFrames = std::stoi(line);
			if (token == "useVIL")
				m_useVIL = std::stoi(line);
			if (token == "useRenderDoc")
				m_useRenderDoc = std::stoi(line);
			if (token == "saveUICommands")
				m_saveUICommands = std::stoi(line);
		}
	}

	configFile.close();
}
