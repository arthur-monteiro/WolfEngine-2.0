#include "AndroidCacheHelper.h"

#ifdef __ANDROID__

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <Debug.h>

void Wolf::copyCompressedFileToStorage(const std::string& filename, const std::string& folderInStorage, std::string& outFilename)
{
    Wolf::Debug::sendInfo("Request uncompression for " + filename);

    std::ifstream appFile("/proc/self/cmdline");
    std::string processName;
    std::getline(appFile, processName);

    std::string appFolderName = "/data/data/" + processName.substr(0, processName.find('\0'));
    appFolderName += "/" + folderInStorage;

    std::filesystem::create_directories(appFolderName);

    std::string escapedFilename;
    for (char c : filename)
    {
        if (c == '/')
            escapedFilename += '_';
        else
            escapedFilename += c;
    }
    outFilename = appFolderName + "/" + escapedFilename;

    Wolf::Debug::sendInfo("File will be " + outFilename);

    if (std::filesystem::exists(outFilename))
    {
        Wolf::Debug::sendInfo("File already exists");
        return;
    }

    AAsset* file = AAssetManager_open(g_configuration->getAndroidAssetManager(), filename.c_str(), AASSET_MODE_BUFFER);
    if (!file)
    {
        Debug::sendCriticalError("Can't uncompress file " + filename);
        return;
    }

    size_t file_length = AAsset_getLength(file);
    std::vector<uint8_t> data(file_length);

    AAsset_read(file, data.data(), file_length);
    AAsset_close(file);

    std::fstream outCacheFile(outFilename, std::ios::out | std::ios::binary);
    outCacheFile.write(reinterpret_cast<char*>(data.data()), data.size());
    outCacheFile.close();
}

#endif // __ANDROID__