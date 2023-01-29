#include "ShaderParser.h"

#include <cctype>
#include <fstream>
#include <sstream>

#include "Debug.h"
#include "ShaderCommon.h"

Wolf::ShaderParser::ShaderParser(const std::string& filename)
{
    m_filename = filename;

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

void Wolf::ShaderParser::readCompiledShader(std::vector<char>& shaderCode)
{
    readFile(shaderCode, m_compileFilename);
}

void Wolf::ShaderParser::parseAndCompile()
{
    std::ifstream inFile(m_filename);

    std::vector<std::string> extensions = { ".vert", ".frag", ".comp"};

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
        Debug::sendError("Extension not handle in shader " + m_filename);

    std::string compiledFilename = parsedFilename;

    parsedFilename += "Parsed" + extensionFound;

    extensionFound.erase(0, 1);
    extensionFound[0] = std::toupper(extensionFound[0]);
    compiledFilename += extensionFound + ".spv";

    std::ofstream outFileGLSL(parsedFilename);

    std::string inShaderLine;
    while (std::getline(inFile, inShaderLine))
    {
        if (inShaderLine == "#include \"ShaderCommon.glsl\"")
        {
            outFileGLSL << shaderCommonStr;
        }
        else
        {
            outFileGLSL << inShaderLine << std::endl;
        }
    }

    outFileGLSL.close();

    std::string commandToCompile = "glslangValidator.exe -V ";
    commandToCompile += parsedFilename;
    commandToCompile += " -o ";
    commandToCompile += compiledFilename;

    system(commandToCompile.c_str());

    m_compileFilename = compiledFilename;
    m_lastModifiedTime = std::filesystem::last_write_time(m_filename);
}

void Wolf::ShaderParser::readFile(std::vector<char>& output, const std::string& filename)
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
