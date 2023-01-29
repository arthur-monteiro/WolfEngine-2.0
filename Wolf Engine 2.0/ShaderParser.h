#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Wolf
{
	class ShaderParser
	{
	public:
		ShaderParser(const std::string& filename);

		bool compileIfFileHasBeenModified();
		void readCompiledShader(std::vector<char>& shaderCode);

	private:
		void parseAndCompile();
		void readFile(std::vector<char>& output, const std::string& filename);

	private:
		std::string m_filename;
		std::filesystem::file_time_type m_lastModifiedTime;
		std::string m_compileFilename;
	};
}