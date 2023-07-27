#include "ShaderParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#ifdef __ANDROID__
#include <AndroidCacheHelper.h>
#endif

#include "Configuration.h"
#include "Debug.h"
#include "ShaderCommon.h"

Wolf::ShaderParser::ShaderParser(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude)
{
    m_filename = filename;
    m_filenamesWithLastModifiedTime.insert({ filename, std::filesystem::last_write_time(filename) });
    m_conditionBlocksToInclude = conditionBlocksToInclude;

    parseAndCompile();
}

bool Wolf::ShaderParser::compileIfFileHasBeenModified(const std::vector<std::string>& conditionBlocksToInclude)
{
    bool needToRecompile = false;
    if(conditionBlocksToInclude != m_conditionBlocksToInclude)
    {
        needToRecompile = true;
        m_conditionBlocksToInclude = conditionBlocksToInclude;
    }

    for(std::map<std::string, std::filesystem::file_time_type>::value_type& file : m_filenamesWithLastModifiedTime)
    {
	    if(std::filesystem::file_time_type time = std::filesystem::last_write_time(file.first); file.second != time)
	    {
            needToRecompile = true;
            file.second = time;
	    }
    }

    if (needToRecompile)
    {
        parseAndCompile();
        return true;
    }
    return false;
}

void Wolf::ShaderParser::readCompiledShader(std::vector<char>& shaderCode) const
{
    readFile(shaderCode, m_compileFilename);
}

void Wolf::ShaderParser::parseAndCompile()
{
#ifdef __ANDROID__
    const std::string appFolderName = "shader_cache";
    copyCompressedFileToStorage(m_filename, appFolderName, m_filename);
#endif

    std::ifstream inFile(m_filename);

    std::vector<std::string> extensions = { ".vert", ".frag", ".comp", ".rgen", ".rmiss", ".rchit" };

    std::string parsedFilename = m_filename;

    std::string extensionFound;
    for (std::string& extension : extensions)
    {
        size_t extensionPosInStr = parsedFilename.find(extension);
        if (extensionPosInStr != std::string::npos)
        {
            extensionFound = extension;
            parsedFilename.erase(extensionPosInStr, extension.size());
        }
    }

    if (extensionFound.empty())
    {
        Debug::sendError("Extension not handle in shader " + m_filename);
    }

    for (const std::string& condition : m_conditionBlocksToInclude)
    {
        parsedFilename += "_" + condition;
    }

    std::string compiledFilename = parsedFilename;
    parsedFilename += "Parsed" + extensionFound;

    extensionFound.erase(0, 1);
    extensionFound[0] = std::toupper(extensionFound[0]);
    compiledFilename += extensionFound + ".spv";

    std::ofstream outFileGLSL(parsedFilename);

    std::vector<std::string> currentConditions;
    std::string inShaderLine;
    while (std::getline(inFile, inShaderLine))
    {
        if (inShaderLine.find("#endif") != std::string::npos)
        {
            currentConditions.pop_back();
            continue;
        }
        else if (size_t tokenPos = inShaderLine.find("#else"); tokenPos != std::string::npos)
        {
            std::string conditionStr = currentConditions.back();
            currentConditions.pop_back();
            currentConditions.push_back("!" + conditionStr);
            continue;
        }

        if (!isRespectingConditions(currentConditions))
            continue;

        if (inShaderLine == "#include \"ShaderCommon.glsl\"")
        {
            outFileGLSL << shaderCommonStr;
        }
        else if(size_t tokenPos = inShaderLine.find("#include "); tokenPos != std::string::npos)
        {
            std::string includeFilename = inShaderLine.substr(tokenPos + 9);
            std::erase(includeFilename, '"');
            std::erase(includeFilename, ' ');

            size_t found = m_filename.find_last_of("/\\");
            std::string folderPath = m_filename.substr(0, found);
            std::string fullIncludeFilename = folderPath + '/' + includeFilename;
            if (!m_filenamesWithLastModifiedTime.contains(fullIncludeFilename))
            {
                m_filenamesWithLastModifiedTime.insert({ fullIncludeFilename, std::filesystem::last_write_time(fullIncludeFilename) });
            }

            // includes seems to work with glslc so no need to copy content
            outFileGLSL << inShaderLine << std::endl;
        }
        else if (size_t tokenPos = inShaderLine.find("#if "); tokenPos != std::string::npos)
        {
            std::string conditionStr = inShaderLine.substr(tokenPos + 4);
            std::erase(conditionStr, ' ');
            currentConditions.push_back(conditionStr);
        }
        else
        {
            outFileGLSL << inShaderLine << std::endl;
        }
    }

    outFileGLSL.close();

#ifndef __ANDROID__
    std::string commandToCompile = "glslc.exe ";
    commandToCompile += parsedFilename;
    commandToCompile += " -o ";
    commandToCompile += compiledFilename;
    commandToCompile += " --target-spv=spv1.4";
    if(g_configuration->getUseRenderDoc())
    {
        commandToCompile += " -g";
    }
    std::system(commandToCompile.c_str());
#else
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc_shader_kind shaderKind;
    if(extensionFound == "Vert")
    {
        shaderKind = shaderc_vertex_shader;
    }
    else if(extensionFound == "Frag")
    {
        shaderKind = shaderc_fragment_shader;
    }
    else
    {
        raise(SIGTRAP);
    }

    std::vector<char> parsedShaderCode;
    readFile(parsedShaderCode, parsedFilename);
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(parsedShaderCode.data(), parsedShaderCode.size(), shaderKind,
                                                                     compiledFilename.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        Debug::sendError(module.GetErrorMessage());
    }

    std::vector<uint32_t> compiledContent(module.cbegin(), module.cend());
    std::fstream outCompiledFile(compiledFilename, std::ios::out | std::ios::binary);
    outCompiledFile.write((char*)compiledContent.data(), compiledContent.size() * sizeof(uint32_t));
    outCompiledFile.close();
#endif

    m_compileFilename = compiledFilename;
}

void Wolf::ShaderParser::readFile(std::vector<char>& output, const std::string& filename) const
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        Debug::sendError("Error opening file : " + filename);

    const size_t fileSize = std::filesystem::file_size(filename);
    output.resize(fileSize);

    file.seekg(0);
    file.read(output.data(), fileSize);

    file.close();
}

bool Wolf::ShaderParser::isRespectingConditions(const std::vector<std::string>& conditions)
{
#ifndef __ANDROID__
    return std::ranges::all_of(conditions.begin(), conditions.end(), [this](const std::string& condition)
		{
			if(condition[0] == '!') // condition must not be included
			{
                std::string conditionCpy = condition;
                std::erase(conditionCpy, '!');
				return std::ranges::find(m_conditionBlocksToInclude, conditionCpy) == m_conditionBlocksToInclude.end();
			}
            else
            {
                return std::ranges::find(m_conditionBlocksToInclude, condition) != m_conditionBlocksToInclude.end();
            }
		});
#else
    if(!conditions.empty())
    {
        Debug::sendError("Permutations are not available on android (std::ranges not supported)");
    }
#endif
}
