#include "ConfigurationHelper.h"

#include <fstream>

void removeSpaces(std::string& line)
{
	std::erase_if(line, isspace);
};

std::string Wolf::ConfigurationHelper::readInfoFromFile(const std::string& filepath, const std::string& token)
{
	std::ifstream inConfigFile(filepath);
	std::string line;
	while (std::getline(inConfigFile, line))
	{
		if (const size_t pos = line.find('='); pos != std::string::npos)
		{
			std::string lineToken = line.substr(0, pos);
			line.erase(0, pos + 1);

			removeSpaces(lineToken);
			removeSpaces(line);

			if (lineToken == token)
				return line;
		}
	}

	return "";
}

void Wolf::ConfigurationHelper::writeInfoToFile(const std::string& filepath, const std::string& token, const std::string& value)
{
	std::ifstream inConfigFile(filepath);
	std::string line;
	std::string fullFile;
	bool tokenFound = false;
	while (std::getline(inConfigFile, line))
	{
		if (const size_t pos = line.find('='); pos != std::string::npos)
		{
			std::string lineToken = line.substr(0, pos);
			removeSpaces(lineToken);

			if (lineToken == token)
			{
				line = token + " = " + value;
				tokenFound = true;
			}
		}

		fullFile += line;
	}

	inConfigFile.close();

	if (!tokenFound)
	{
		if (!fullFile.empty() && fullFile.back() != '\n')
		{
			fullFile += '\n';
		}
		fullFile += token + " = " + value + '\n';
	}

	std::ofstream outConfigFile(filepath);
	outConfigFile << fullFile;
	outConfigFile.close();
}

void Wolf::ConfigurationHelper::writeInfoToFile(const std::string& filepath, const std::string& token, bool value)
{
	writeInfoToFile(filepath, token, static_cast<const std::string&>(value ? "true" : "false"));
}
