#include "AndroidCacheHelper.h"

#ifdef __ANDROID__

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

void Wolf::copyCompressedFileToStorage(const std::string& filename, const std::string& folderInStorage, std::string& outFilename)
{
    std::ifstream appFile("/proc/self/cmdline");
    std::string processName;
    std::getline(appFile, processName);
    std::string appFolderName = "/data/data/" + processName.substr(0, processName.find('\0'));
    appFolderName += "/" + folderInStorage;
    std::filesystem::create_directory(appFolderName);

    AAsset* file = AAssetManager_open(g_configuration->getAndroidAssetManager(), filename.c_str(), AASSET_MODE_BUFFER);
    size_t file_length = AAsset_getLength(file);

    std::vector<uint8_t> data;
    data.resize(file_length);

    AAsset_read(file, data.data(), file_length);
    AAsset_close(file);

    std::string filenameWithoutPath = filename.substr(filename.find_last_of("/") + 1);
    outFilename = appFolderName + "/" + filenameWithoutPath;
    std::fstream outCacheFile(outFilename, std::ios::out | std::ios::binary);
    outCacheFile.write((char*)data.data(), data.size());
    outCacheFile.close();
}

#endif // __ANDROID__