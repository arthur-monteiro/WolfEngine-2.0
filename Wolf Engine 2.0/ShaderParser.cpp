#include "ShaderParser.h"

#include <cctype>
#include <fstream>
#include <sstream>

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

    std::string commandToCompile = "glslc.exe ";
    commandToCompile += parsedFilename;
    commandToCompile += " -o ";
    commandToCompile += compiledFilename;
    commandToCompile += " --target-spv=spv1.4";

    system(commandToCompile.c_str());

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
