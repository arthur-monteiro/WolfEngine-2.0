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

			AddShaderInfo(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude = defaultConditionBlocksToInclude) : filename(filename), conditionBlocksToInclude(conditionBlocksToInclude) {}
		};
		const ShaderParser* addShader(const AddShaderInfo& addShaderInfo);

		void checkForModifiedShader() const;

	private:
		class ShaderInfo
		{
		public:
			ShaderInfo(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude, uint32_t cameraDescriptorSlot)
			{
				m_shaderParser.reset(new ShaderParser(filename, conditionBlocksToInclude, cameraDescriptorSlot));
			}

			std::vector<std::function<void(const ShaderParser*)>>& getCallbackWhenModifiedList() { return m_callbackWhenModified; }
			const std::vector<std::function<void(const ShaderParser*)>>& getCallbackWhenModifiedList() const { return m_callbackWhenModified; }
			ShaderParser* getShaderParser() const { return m_shaderParser.get(); }

		private:
			std::unique_ptr<ShaderParser> m_shaderParser;
			std::vector<std::function<void(const ShaderParser*)>> m_callbackWhenModified;
		};
		std::vector<ShaderInfo> m_shaderInfos;
	};

	extern ShaderList* g_shaderList;
}