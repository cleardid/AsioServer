
#ifndef PATHUTILS_H
#define PATHUTILS_H

#include <filesystem>

namespace util
{
    std::filesystem::path GetExecutablePath();
    std::filesystem::path GetExecutableDir();
}

#endif // PATHUTILS_H