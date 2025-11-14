#pragma once

#include <functional>

#include "ShaderParser.h"

namespace Wolf
{
	class ShaderList
	{
	public:
		inline static const std::vector<std::string> defaultConditionBlocksToInclude = {};
		struct AddShaderInfo
		{
			const std::string& filename;
			const std::vector<std::string>& conditionBlocksToInclude;

			std::vector<std::function<void(const ShaderParser*)>> callbackWhenModified;
			uint32_t cameraDescriptorSlot = -1;
			uint32_t bindlessDescriptorSlot = -1;
			uint32_t lightDescriptorSlot = -1;
			ShaderParser::MaterialFetchProcedure materialFetchProcedure;
			ShaderParser::ShaderCodeToAdd shaderCodeToAdd;

			AddShaderInfo(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude = defaultConditionBlocksToInclude) : filename(filename), conditionBlocksToInclude(conditionBlocksToInclude) {}
		};
		const ShaderParser* addShader(const AddShaderInfo& addShaderInfo);

		void checkForModifiedShader() const;

	private:
		class ShaderInfo
		{
		public:
			ShaderInfo(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude, uint32_t cameraDescriptorSlot, uint32_t bindlessDescriptorSlot, uint32_t lightDescriptorSlot,
				const ShaderParser::MaterialFetchProcedure& materialFetchProcedure, const ShaderParser::ShaderCodeToAdd& shaderCodeToAdd)
			{
				m_shaderParser.reset(new ShaderParser(filename, conditionBlocksToInclude, cameraDescriptorSlot, bindlessDescriptorSlot, lightDescriptorSlot, materialFetchProcedure, shaderCodeToAdd));
			}

			std::vector<std::function<void(const ShaderParser*)>>& getCallbackWhenModifiedList() { return m_callbackWhenModified; }
			const std::vector<std::function<void(const ShaderParser*)>>& getCallbackWhenModifiedList() const { return m_callbackWhenModified; }
			ShaderParser* getShaderParser() const { return m_shaderParser.get(); }

		private:
			std::unique_ptr<ShaderParser> m_shaderParser;
			std::vector<std::function<void(const ShaderParser*)>> m_callbackWhenModified;
		};
		std::vector<ShaderInfo> m_shadersInfo;
	};
}
