#include "ShaderParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/native_activity.h>
#include <shaderc/shaderc.hpp>
#endif

#include "Configuration.h"
#include "Debug.h"
#include "ShaderCommon.h"

Wolf::ShaderParser::ShaderParser(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude)
{
    m_filename = filename;
    m_conditionBlocksToInclude = conditionBlocksToInclude;

    parseAndCompile();
}

bool Wolf::ShaderParser::compileIfFileHasBeenModified()
{
    if (m_lastModifiedTime != std::filesystem::last_write_time(m_filename))
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
    std::ifstream appFile("/proc/self/cmdline");
    std::string processName;
    std::getline(appFile, processName);
    std::string appFolderName = "/data/data/" + processName.substr(0, processName.find('\0'));
    appFolderName += "/shader_cache";
    std::filesystem::create_directory(appFolderName);

    AAsset *file = AAssetManager_open(g_configuration->getAndroidAssetManager(), m_filename.c_str(), AASSET_MODE_BUFFER);
    size_t file_length = AAsset_getLength(file);

    std::vector<uint8_t> data;
    data.resize(file_length);

    AAsset_read(file, data.data(), file_length);
    AAsset_close(file);

    std::string shaderFilename = m_filename.substr(m_filename.find_last_of("/") + 1);
    m_filename = appFolderName + "/" + shaderFilename;
    std::fstream outCacheFile(m_filename, std::ios::out | std::ios::binary);
    outCacheFile.write((char*)data.data(), data.size());
    outCacheFile.close();
#endif

    std::ifstream inFile(m_filename);

    std::vector<std::string> extensions = { ".vert", ".frag", ".comp", ".rgen", ".rmiss", ".rchit" };

    std::string parsedFilename = m_filename;

    std::string extensionFound = "";
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

        if (!isRespectingConditions(currentConditions))
            continue;

        if (inShaderLine == "#include \"ShaderCommon.glsl\"")
        {
            outFileGLSL << shaderCommonStr;
        }
        else if (size_t tokenPos = inShaderLine.find("#if "); tokenPos != std::string::npos)
        {
            std::string conditionStr = inShaderLine.substr(tokenPos + 4);
            conditionStr.erase(std::remove(conditionStr.begin(), conditionStr.end(), ' '), conditionStr.end());
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
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(parsedShaderCode.data(), data.size(), shaderKind,
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
    m_lastModifiedTime = std::filesystem::last_write_time(m_filename);
}

void Wolf::ShaderParser::readFile(std::vector<char>& output, const std::string& filename) const
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        Debug::sendError("Error opening file : " + filename);

    size_t fileSize = std::filesystem::file_size(filename);
    output.resize(fileSize);

    file.seekg(0);
    file.read(output.data(), fileSize);

    file.close();
}

bool Wolf::ShaderParser::isRespectingConditions(const std::vector<std::string>& conditions)
{
    for (const std::string& condition : conditions)
    {
        if (std::find(m_conditionBlocksToInclude.begin(), m_conditionBlocksToInclude.end(), condition) == m_conditionBlocksToInclude.end())
            return false;
    }
    return true;
}
