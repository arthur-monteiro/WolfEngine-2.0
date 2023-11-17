#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace Wolf
{
	class ShaderParser
	{
	public:
		ShaderParser(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude = {}, uint32_t cameraDescriptorSlot = -1);

		bool compileIfFileHasBeenModified(const std::vector<std::string>& conditionBlocksToInclude = {});
		void readCompiledShader(std::vector<char>& shaderCode) const;

		const std::vector<std::string>& getCurrentConditionsBlocks() const { return m_conditionBlocksToInclude; }
		bool isSame(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude) const;

	private:
		void parseAndCompile();
		void readFile(std::vector<char>& output, const std::string& filename) const;
		bool isRespectingConditions(const std::vector<std::string>& conditions);
		void addCameraCode(std::ofstream& outFileGLSL) const;

	private:
		std::string m_filename;
		std::map<std::string, std::filesystem::file_time_type> m_filenamesWithLastModifiedTime;
		std::vector<std::string> m_conditionBlocksToInclude;
		uint32_t m_cameraDescriptorSlot;
		std::string m_compileFilename;
	};
}
