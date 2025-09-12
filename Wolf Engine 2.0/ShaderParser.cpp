#include "ShaderParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#ifdef __ANDROID__
#include <AndroidCacheHelper.h>
#endif

#include <xxh64.hpp>

#include "Configuration.h"
#include "Debug.h"
#include "ShaderCommon.h"

uint64_t Wolf::ShaderParser::MaterialFetchProcedure::computeHash() const
{
    if (codeString.empty())
        return 0;
    return xxh64::hash(codeString.c_str(), codeString.length(), 0);
}

uint64_t Wolf::ShaderParser::ShaderCodeToAdd::computeHash() const
{
    if (codeString.empty())
        return 0;

    return xxh64::hash(codeString.c_str(), codeString.length(), 0);
}

#ifndef __ANDROID__
bool Wolf::ShaderParser::ShaderCodeToAdd::lookForRayTraceCall() const
{
    return codeString.contains("traceRayEXT(");
}
#endif

void Wolf::ShaderParser::ShaderCodeToAdd::addToGLSL(std::ofstream& outFileGLSL) const
{
    outFileGLSL << codeString;
}

Wolf::ShaderParser::ShaderParser(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude, uint32_t cameraDescriptorSlot, uint32_t bindlessDescriptorSlot, uint32_t lightDescriptorSlot, 
                                 const MaterialFetchProcedure& materialFetchProcedure, const ShaderCodeToAdd& shaderCodeToAdd)
{
    m_filename = filename;
#ifndef __ANDROID__
    m_filenamesWithLastModifiedTime.insert({ filename, std::filesystem::last_write_time(filename) });
#endif
    m_conditionBlocksToInclude = conditionBlocksToInclude;

    m_cameraDescriptorSlot = cameraDescriptorSlot;
    m_bindlessDescriptorSlot = bindlessDescriptorSlot;
    m_lightDescriptorSlot = lightDescriptorSlot;
    m_materialFetchProcedure = materialFetchProcedure;
    m_materialFetchProcedureHash = materialFetchProcedure.computeHash();
    m_shaderCodeToAdd = shaderCodeToAdd;
    m_shaderCodeToAddHash = shaderCodeToAdd.computeHash();

    parseAndCompile();
}

bool Wolf::ShaderParser::compileIfFileHasBeenModified(const std::vector<std::string>& conditionBlocksToInclude)
{
#ifdef __ANDROID__
    return false;
#endif

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

bool Wolf::ShaderParser::isSame(const std::string& filename, const std::vector<std::string>& conditionBlocksToInclude, uint64_t materialFetchProcedureHash, uint64_t shaderCodeToAddHash) const
{
    return filename == m_filename && conditionBlocksToInclude == m_conditionBlocksToInclude && m_materialFetchProcedureHash == materialFetchProcedureHash && m_shaderCodeToAddHash == shaderCodeToAddHash;
}

void Wolf::ShaderParser::parseAndCompile()
{
#ifdef __ANDROID__
    const std::string appFolderName = "shader_cache";
    copyCompressedFileToStorage(m_filename, appFolderName, m_filename);
#endif

    std::ifstream inFile(m_filename);

    std::vector<std::string> extensions = { ".vert", ".frag", ".comp", ".rgen", ".rmiss", ".rchit", ".tesc", ".tese" };

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

    if (m_materialFetchProcedureHash != 0)
    {
        parsedFilename += "_" + std::to_string(m_materialFetchProcedureHash);
    }

    if (m_shaderCodeToAddHash != 0)
    {
        parsedFilename += "_" + std::to_string(m_shaderCodeToAddHash);
    }

    if (g_configuration->getUseVirtualTexture())
    {
        parsedFilename += "_virtualTexture";
    }

    std::string compiledFilename = parsedFilename;
    parsedFilename += "Parsed" + extensionFound;

    extensionFound.erase(0, 1);
    extensionFound[0] = std::toupper(extensionFound[0]);
    compiledFilename += extensionFound + ".spv";

    std::ofstream outFileGLSL(parsedFilename);

    outFileGLSL << "#version 460\n";

    outFileGLSL << "#define GLSL\n\n";
    if (extensionFound == "Comp")
    {
        outFileGLSL << "#define COMPUTE_SHADER\n";
    }

    for (const std::string& condition : m_conditionBlocksToInclude)
    {
        outFileGLSL << "#define " + condition + "\n";
    }

    outFileGLSL << "\n";

    addCameraCode(outFileGLSL);
    if (extensionFound == "Frag" || extensionFound == "Rchit")
    {
        addMaterialFetchCode(outFileGLSL);
        addLightInfoCode(outFileGLSL);
    }

#ifndef __ANDROID__
    bool addRayTracedShaderCode = false;
#endif
    if (m_shaderCodeToAddHash)
    {
#ifndef __ANDROID__
        if (m_shaderCodeToAdd.lookForRayTraceCall())
        {
            // We'll need to add the code after the payload definition
            addRayTracedShaderCode = true;
        }
        else
#endif
        {
            m_shaderCodeToAdd.addToGLSL(outFileGLSL);
        }
    }

    std::vector<std::string> currentConditions;
    std::string inShaderLine;
    while (std::getline(inFile, inShaderLine))
    {
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
        else
        {
            outFileGLSL << inShaderLine << std::endl;

#ifndef __ANDROID__
            if (addRayTracedShaderCode && (inShaderLine.contains("rayPayloadEXT") || inShaderLine.contains("rayPayloadInEXT")))
            {
                m_shaderCodeToAdd.addToGLSL(outFileGLSL);
                addRayTracedShaderCode = false;
            }
#endif
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

void Wolf::ShaderParser::addCameraCode(std::ofstream& outFileGLSL) const
{
    if (m_cameraDescriptorSlot == static_cast<uint32_t>(-1))
        return;

    std::string cameraCode = 
		#include "Camera.glsl"
    ;

    const std::string& descriptorSlotToken = "£CAMERA_DESCRIPTOR_SLOT";
    if (const size_t descriptorSlotTokenPos = cameraCode.find(descriptorSlotToken); descriptorSlotTokenPos != std::string::npos)
    {
        cameraCode.replace(descriptorSlotTokenPos, descriptorSlotToken.length(), std::to_string(m_cameraDescriptorSlot));
    }

    outFileGLSL << cameraCode;
}

void Wolf::ShaderParser::addMaterialFetchCode(std::ofstream& outFileGLSL) const
{
    if (m_bindlessDescriptorSlot == static_cast<uint32_t>(-1))
        return;

    outFileGLSL << "\n//------------------\n";

    std::string materialFetchCode;

    if (g_configuration->getUseVirtualTexture())
    {
        materialFetchCode +=
            #include "VirtualTextureUtils.glsl"
        ;

        materialFetchCode +=
            #include "VirtualTextureSample.glsl"
        ;
    }
    else
    {
        materialFetchCode +=
            #include "DefaultTextureSample.glsl"
        ;
    }

    if (m_materialFetchProcedure.codeString.empty())
    {
        materialFetchCode += "#define DEFAULT_MATERIAL_FETCH\n";
        materialFetchCode +=
			#include "DefaultMaterialFetch.glsl"
        ;
        outFileGLSL << "// DefaultMaterialFetch.glsl\n";
    }
    else
    {
        materialFetchCode +=
			#include "DefaultMaterialFetch.glsl"
        ;
        materialFetchCode += m_materialFetchProcedure.codeString;
        outFileGLSL << "// Custom" << '\n';
    }

    outFileGLSL << "//------------------\n";

    const std::string& descriptorSlotToken = "£BINDLESS_DESCRIPTOR_SLOT";
    size_t descriptorSlotTokenPos = materialFetchCode.find(descriptorSlotToken);
    while (descriptorSlotTokenPos != std::string::npos)
    {
        materialFetchCode.replace(descriptorSlotTokenPos, descriptorSlotToken.length(), std::to_string(m_bindlessDescriptorSlot));
        descriptorSlotTokenPos = materialFetchCode.find(descriptorSlotToken);
    }

    outFileGLSL << materialFetchCode;
}

void Wolf::ShaderParser::addLightInfoCode(std::ofstream& outFileGLSL) const
{
    if (m_lightDescriptorSlot == static_cast<uint32_t>(-1))
        return;

    std::string lightInfoCode =
		#include "LightInfo.glsl"
    ;

    const std::string& descriptorSlotToken = "£LIGHT_INFO_DESCRIPTOR_SLOT";
    if (const size_t descriptorSlotTokenPos = lightInfoCode.find(descriptorSlotToken); descriptorSlotTokenPos != std::string::npos)
    {
        lightInfoCode.replace(descriptorSlotTokenPos, descriptorSlotToken.length(), std::to_string(m_lightDescriptorSlot));
    }

    outFileGLSL << lightInfoCode;
}
