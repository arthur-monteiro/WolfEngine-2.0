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
		struct MaterialFetchProcedure
		{
			std::string codeString;

			uint64_t computeHash() const;
		};
		struct ShaderCodeToAdd
		{
			std::string codeString;

			uint64_t computeHash() const;
#ifndef __ANDROID__
			bool lookForRayTraceCall() const;
#endif
			void addToGLSL(std::ofstream& outFileGLSL) const;
		};
		ShaderParser(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude = {}, uint32_t cameraDescriptorSlot = -1, uint32_t materialsDescriptorSlot = -1, uint32_t lightDescriptorSlot = -1, const MaterialFetchProcedure& materialFetchProcedure = MaterialFetchProcedure(),
			const ShaderCodeToAdd& shaderCodesToAdd = ShaderCodeToAdd());

		bool compileIfFileHasBeenModified(const std::vector<std::string>& conditionBlocksToInclude = {});
		void readCompiledShader(std::vector<char>& shaderCode) const;

		const std::vector<std::string>& getCurrentConditionsBlocks() const { return m_conditionBlocksToInclude; }
		bool isSame(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude, uint64_t materialFetchProcedureHash, uint64_t shaderCodeToAddHash) const;

	private:
		void parseAndCompile();
		void readFile(std::vector<char>& output, const std::string& filename) const;
		void addCameraCode(std::ofstream& outFileGLSL) const;
		void addMaterialFetchCode(std::ofstream& outFileGLSL) const;
		void addLightInfoCode(std::ofstream& outFileGLSL) const;

	private:
		std::string m_filename;
		std::map<std::string, std::filesystem::file_time_type> m_filenamesWithLastModifiedTime;
		std::vector<std::string> m_conditionBlocksToInclude;

		uint32_t m_cameraDescriptorSlot;
		uint32_t m_materialsDescriptorSlot;
		uint32_t m_lightDescriptorSlot;
		MaterialFetchProcedure m_materialFetchProcedure;
		uint64_t m_materialFetchProcedureHash = 0;
		ShaderCodeToAdd m_shaderCodeToAdd;
		uint64_t m_shaderCodeToAddHash = 0;

		std::string m_compileFilename;
	};
}
