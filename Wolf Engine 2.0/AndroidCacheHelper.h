#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/native_activity.h>

#include "Configuration.h"

namespace Wolf
{
    void copyCompressedFileToStorage(const std::string& filename, const std::string& folderInStorage, std::string& outFilename);
}
#endif // __ANDROID__