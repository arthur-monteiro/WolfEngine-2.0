#include "ShaderList.h"

#include "ProfilerCommon.h"

const Wolf::ShaderParser* Wolf::ShaderList::addShader(const AddShaderInfo& addShaderInfo)
{
	for (const ShaderInfo& shaderInfo : m_shadersInfo)
	{
		if (shaderInfo.getShaderParser()->isSame(addShaderInfo.filename, addShaderInfo.conditionBlocksToInclude, addShaderInfo.materialFetchProcedure.computeHash(), addShaderInfo.shaderCodeToAdd.computeHash()))
		{
			return shaderInfo.getShaderParser();
		}
	}

	m_shadersInfo.emplace_back(addShaderInfo.filename, addShaderInfo.conditionBlocksToInclude, addShaderInfo.cameraDescriptorSlot, addShaderInfo.bindlessDescriptorSlot, addShaderInfo.lightDescriptorSlot, addShaderInfo.materialFetchProcedure,
		addShaderInfo.shaderCodeToAdd);
	m_shadersInfo.back().getCallbackWhenModifiedList() = addShaderInfo.callbackWhenModified;

	return m_shadersInfo.back().getShaderParser();
}

void Wolf::ShaderList::checkForModifiedShader() const
{
	PROFILE_FUNCTION

	for (const ShaderInfo& shaderInfo : m_shadersInfo)
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
