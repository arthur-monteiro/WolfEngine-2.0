#include "Configuration.h"

#include <fstream>
#include <sstream>

#include "Debug.h"

Wolf::Configuration* Wolf::g_configuration = nullptr;

Wolf::Configuration::Configuration(const std::string& filename)
{
	if (g_configuration)
		Debug::sendCriticalError("Can't instanciate Configuration twice");

	std::ifstream configFile(filename);
	std::string line;
	while (std::getline(configFile, line))
	{
		if (size_t pos = line.find("="); pos != std::string::npos)
		{
			std::string token = line.substr(0, pos);
			line.erase(0, pos + 1);

			if (token == "windowWidth")
				m_windowWidth = std::stoi(line);
			if (token == "windowHeight")
				m_windowHeight = std::stoi(line);
			if (token == "maxCachedFrames")
				m_maxCachedFrames = std::stoi(line);
		}
	}

	configFile.close();
}
