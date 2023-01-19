#include "TextFileReader.h"

#include <filesystem>
#include <fstream>
#include <sstream>

Wolf::TextFileReader::TextFileReader(std::string filename)
{
	std::ifstream inFile(filename);

    std::string inLine;
    while (std::getline(inFile, inLine))
    {
        m_fileContent += inLine;
    }
}
