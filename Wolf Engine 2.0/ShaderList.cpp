#include "ShaderList.h"

Wolf::ShaderList* Wolf::g_shaderList;

const Wolf::ShaderParser* Wolf::ShaderList::addShader(const AddShaderInfo& addShaderInfo)
{
	for (const ShaderInfo& shaderInfo : m_shaderInfos)
	{
		if (shaderInfo.getShaderParser()->isSame(addShaderInfo.filename, addShaderInfo.conditionBlocksToInclude))
		{
			return shaderInfo.getShaderParser();
		}
	}

	m_shaderInfos.emplace_back(addShaderInfo.filename, addShaderInfo.conditionBlocksToInclude);
	m_shaderInfos.back().getCallbackWhenModifiedList() = addShaderInfo.callbackWhenModified;

	return m_shaderInfos.back().getShaderParser();
}

void Wolf::ShaderList::checkForModifiedShader() const
{
	for (const ShaderInfo& shaderInfo : m_shaderInfos)
	{
		if (shaderInfo.getShaderParser()->compileIfFileHasBeenModified(shaderInfo.getShaderParser()->getCurrentConditionsBlocks()))
		{
			for (auto& callback : shaderInfo.getCallbackWhenModifiedList())
			{
				callback(shaderInfo.getShaderParser());
			}
		}
	}
}
