#include "ImageFileLoader.h"

#include <fstream>
#include <glm/gtc/packing.hpp>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <sstream>

#include <Debug.h>

#include "AndroidCacheHelper.h"

Wolf::ImageFileLoader::ImageFileLoader(const std::string& fullFilePath, bool loadFloat)
{
	std::string fileExtension = fullFilePath.substr(fullFilePath.find_last_of(".") + 1);
    std::string filename = fullFilePath;

#ifdef __ANDROID__
    Wolf::copyCompressedFileToStorage(fullFilePath, "bin_cache", filename);
#endif

	if (fileExtension == "dds")
	{
		loadDDS(filename);
	}
	else if (fileExtension == "cube")
	{
		loadCube(filename);
	}
	else if (fileExtension == "hdr" || loadFloat)
	{
        int iWidth(0), iHeight(0), iChannels(0);
		float* pixels = stbi_loadf(filename.c_str(), &iWidth, &iHeight, &iChannels, STBI_rgb_alpha);
        m_width = static_cast<uint32_t>(iWidth);
        m_height = static_cast<uint32_t>(iHeight);
        m_channels = static_cast<uint32_t>(iChannels);

		m_pixels = reinterpret_cast<unsigned char*>(pixels);
		m_channels = 4;
		m_format = Format::R32G32B32A32_SFLOAT;

		if (!pixels)
			Debug::sendError("Error loading file " + filename + " !");
	}
	else
	{
        int iWidth(0), iHeight(0), iChannels(0);
		stbi_uc* pixels = stbi_load(filename.c_str(), &iWidth, &iHeight, &iChannels, STBI_rgb_alpha);
        m_width = static_cast<uint32_t>(iWidth);
        m_height = static_cast<uint32_t>(iHeight);
        m_channels = static_cast<uint32_t>(iChannels);

		m_pixels = pixels;
		m_channels = 4;
		m_format = Format::R8G8B8A8_UNORM;

		if (!pixels)
			Debug::sendError("Error : loading image " + filename);
	}
}

Wolf::ImageFileLoader::~ImageFileLoader()
{
	stbi_image_free(m_pixels);
}

typedef unsigned int uint32;
typedef uint32 DWORD;

struct DDS_PIXELFORMAT
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwABitMask;
};

struct DDS_HEADER
{
    uint32          size;
    uint32          flags;
    uint32          height;
    uint32          width;
    uint32          pitchOrLinearSize;
    uint32          depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
    uint32          mipMapCount;
    uint32          reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32          caps;
    uint32          caps2;
    uint32          caps3;
    uint32          caps4;
    uint32          reserved2;
};

enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32A32_UINT = 3,
    DXGI_FORMAT_R32G32B32A32_SINT = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32B32_UINT = 7,
    DXGI_FORMAT_R32G32B32_SINT = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R32G32_TYPELESS = 15,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32_UINT = 17,
    DXGI_FORMAT_R32G32_SINT = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    DXGI_FORMAT_R10G10B10A2_UINT = 25,
    DXGI_FORMAT_R11G11B10_FLOAT = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,
    DXGI_FORMAT_R16G16_TYPELESS = 33,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16G16_UINT = 36,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,
    DXGI_FORMAT_R32_TYPELESS = 39,
    DXGI_FORMAT_D32_FLOAT = 40,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R32_SINT = 43,
    DXGI_FORMAT_R24G8_TYPELESS = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DXGI_FORMAT_R8G8_TYPELESS = 48,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R8G8_UINT = 50,
    DXGI_FORMAT_R8G8_SNORM = 51,
    DXGI_FORMAT_R8G8_SINT = 52,
    DXGI_FORMAT_R16_TYPELESS = 53,
    DXGI_FORMAT_R16_FLOAT = 54,
    DXGI_FORMAT_D16_UNORM = 55,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,
    DXGI_FORMAT_R8_TYPELESS = 60,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_R8_UINT = 62,
    DXGI_FORMAT_R8_SNORM = 63,
    DXGI_FORMAT_R8_SINT = 64,
    DXGI_FORMAT_A8_UNORM = 65,
    DXGI_FORMAT_R1_UNORM = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DXGI_FORMAT_BC6H_TYPELESS = 94,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_TYPELESS = 97,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    DXGI_FORMAT_AYUV = 100,
    DXGI_FORMAT_Y410 = 101,
    DXGI_FORMAT_Y416 = 102,
    DXGI_FORMAT_NV12 = 103,
    DXGI_FORMAT_P010 = 104,
    DXGI_FORMAT_P016 = 105,
    DXGI_FORMAT_420_OPAQUE = 106,
    DXGI_FORMAT_YUY2 = 107,
    DXGI_FORMAT_Y210 = 108,
    DXGI_FORMAT_Y216 = 109,
    DXGI_FORMAT_NV11 = 110,
    DXGI_FORMAT_AI44 = 111,
    DXGI_FORMAT_IA44 = 112,
    DXGI_FORMAT_P8 = 113,
    DXGI_FORMAT_A8P8 = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM = 115,
    DXGI_FORMAT_P208 = 130,
    DXGI_FORMAT_V208 = 131,
    DXGI_FORMAT_V408 = 132,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
    DXGI_FORMAT_FORCE_UINT = 0xffffffff
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT     dxgiFormat;
    uint32_t        resourceDimension;
    uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t        arraySize;
    uint32_t        miscFlags2; // see DDS_MISC_FLAGS2
};

#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
	(static_cast<uint32_t>(static_cast<uint8_t>(ch0)) \
    | (static_cast<uint32_t>(static_cast<uint8_t>(ch1)) << 8) \
    | (static_cast<uint32_t>(static_cast<uint8_t>(ch2)) << 16) \
	| (static_cast<uint32_t>(static_cast<uint8_t>(ch3)) << 24))

const DWORD D3DFMT_DXT1 = '1TXD';
const DWORD D3DFMT_DXT2 = '2TXD';
const DWORD D3DFMT_DXT3 = '3TXD';
const DWORD D3DFMT_DXT4 = '4TXD';
const DWORD D3DFMT_DXT5 = '5TXD';
const DWORD BC5U = MAKEFOURCC('B', 'C', '5', 'U');
const DWORD BC5S = MAKEFOURCC('B', 'C', '5', 'S');
const DWORD ATI2 = MAKEFOURCC('A', 'T', 'I', '2');
const DWORD DX10 = MAKEFOURCC('D', 'X', '1', '0');

void Wolf::ImageFileLoader::loadDDS(const std::string& fullFilePath)
{
    std::ifstream infile;
    infile.open(fullFilePath, std::ios::in | std::ios::binary);

    if (!infile.is_open())
    {
        Debug::sendError("Error : loading image " + fullFilePath);
        return;
    }

    infile.seekg(4, std::ios::beg); // skip magic word

    DDS_HEADER header{};
    infile.read(reinterpret_cast<char*>(&header), sizeof(DDS_HEADER));

    m_width = header.width;
    m_height = header.height;
    
    switch (header.ddspf.dwFourCC)
    {
    case D3DFMT_DXT1:
        m_compression = ImageCompression::Compression::BC1;
        m_format = Format::BC1_RGB_SRGB_BLOCK;
        break;
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
        m_compression = ImageCompression::Compression::BC2;
        break;
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        m_compression = ImageCompression::Compression::BC3;
        break;
    case BC5U:
    case ATI2:
        m_compression = ImageCompression::Compression::BC5;
        break;
    case DX10:
	{
        infile.seekg(sizeof(DDS_HEADER) + 4, std::ios::beg);

        DDS_HEADER_DXT10 dxt10Header{};
        infile.read(reinterpret_cast<char*>(&dxt10Header), sizeof(dxt10Header));

        switch (dxt10Header.dxgiFormat)
        {
			case DXGI_FORMAT_R8G8B8A8_UNORM:
                m_format = Format::R8G8B8A8_UNORM;
				break;
        	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                m_format = Format::R8G8B8A8_SRGB;
                break;
	        case DXGI_FORMAT_BC5_UNORM:
	            m_compression = ImageCompression::Compression::BC5;
                break;
	        default:
	            Debug::sendError("Unsupported compression for DX10 DDS " + fullFilePath);
        }
        break;
	}
	default: 
		Debug::sendError("Unsupported compression for " + fullFilePath);
    }

    uint32 minWidth = header.width % 4 != 0 ? (header.width / 4 + 1) * 4 : header.width;
    uint32 minHeight = header.height % 4 != 0 ? (header.height / 4 + 1) * 4 : header.height;

    uint32 blockSize;
    switch (m_compression)
    {
	    case ImageCompression::Compression::NO_COMPRESSION:
            blockSize = 16 * 4;
            break;
	    case ImageCompression::Compression::BC1: 
            blockSize = sizeof(ImageCompression::BC1);
            break;
	    case ImageCompression::Compression::BC2: 
            blockSize = sizeof(ImageCompression::BC2);
    		break;
	    case ImageCompression::Compression::BC3: 
            blockSize = sizeof(ImageCompression::BC3);
    		break;
	    case ImageCompression::Compression::BC5:
            blockSize = sizeof(ImageCompression::BC5);
            break;
    	default:
            Debug::sendError("Unsupported compression");
            return;
    }
    const float bpp = static_cast<float>(blockSize) / 16.0f;

    size_t sizeInBytes = static_cast<size_t>(static_cast<float>(minWidth) * static_cast<float>(minHeight) * bpp);

    m_pixels = new unsigned char[sizeInBytes];
    infile.seekg(header.size + 4, std::ios::beg);
    infile.read(reinterpret_cast<char*>(m_pixels), static_cast<long long>(sizeInBytes));

    uint32_t currentPosInFile = header.size + 4 + static_cast<uint32_t>(sizeInBytes);

    m_mipPixels.resize(header.mipMapCount - 1);
    for (uint32_t i = 1; i < header.mipMapCount; ++i)
    {
        minWidth /= 2;
        minHeight /= 2;

        if (minWidth < 4 || minHeight < 4)
        {
            m_mipPixels.resize(i - 1);
            break;
        }

        sizeInBytes = static_cast<size_t>(static_cast<float>(minWidth) * static_cast<float>(minHeight) * bpp);

        m_mipPixels[i - 1].resize(static_cast<size_t>(static_cast<float>(minWidth) * static_cast<float>(minHeight) * bpp));
        infile.seekg(currentPosInFile, std::ios::beg);
        infile.read(reinterpret_cast<char*>(m_mipPixels[i - 1].data()), static_cast<long long>(sizeInBytes));

        currentPosInFile += static_cast<uint32_t>(sizeInBytes);
    }
}

bool isFloat(std::string str)
{
	std::istringstream iss(str);
	float f;
	iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
	// Check the entire string was consumed and if either failbit or badbit is set
	return iss.eof() && !iss.fail();
}

bool Wolf::ImageFileLoader::loadCube(const std::string& fullFilePath)
{
	uint16_t* data = nullptr;

	std::ifstream file(fullFilePath);
	std::string line;
	uint32 lineIdx = 0;
	uint32 dataIdx = 0;
	bool dataStarted = false;
	while (std::getline(file, line))
	{
		if (const size_t pos = line.find("LUT_3D_SIZE"); pos != std::string::npos)
		{
			line.erase(0, pos + 12);
			uint32_t cubeSize = std::stoi(line);

			m_width = m_height = m_depth = cubeSize;

			size_t sizeInBytes = static_cast<size_t>(cubeSize * cubeSize * cubeSize * 4 /* must be RGBA even if alpha is not used */* 2 /* size of half */);
			m_pixels = new unsigned char[sizeInBytes];
			data = reinterpret_cast<uint16_t*>(m_pixels);

			lineIdx++;
			continue;
		}
		else if (!data)
		{
			continue; // looking for "LUT_3D_SIZE" on next lines
		}
		else if (!dataStarted)
		{
			if (const size_t firstSpace = line.find(' '); firstSpace != std::string::npos)
			{
				if (isFloat(line.substr(0, firstSpace)))
				{
					dataStarted = true;
				}
				else
					continue;
			}
			else
				continue;
		}

		const size_t firstSpace = line.find(' ');
		data[dataIdx++] = glm::packHalf1x16(std::stof(line.substr(0, firstSpace)));
		line.erase(0, firstSpace + 1);

		const size_t secondSpace = line.find(' ');
		data[dataIdx++] = glm::packHalf1x16(std::stof(line.substr(0, secondSpace)));
		line.erase(0, secondSpace + 1);

		data[dataIdx++] =  glm::packHalf1x16(std::stof(line));

		data[dataIdx++] =  glm::packHalf1x16(0.0f); // alpha
	}

	m_channels = 4;
	m_format = Format::R16G16B16A16_SFLOAT;

	return true;
}
