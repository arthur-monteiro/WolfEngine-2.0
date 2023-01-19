#pragma once

#include <string>

namespace Wolf
{
	class TextFileReader
	{
	public:
		TextFileReader(std::string filename);
		TextFileReader(const TextFileReader&) = delete;

		const std::string& getFileContent() const { return m_fileContent; }

	private:
		std::string m_fileContent;
	};
}

