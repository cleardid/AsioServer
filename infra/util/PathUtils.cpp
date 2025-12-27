#include "PathUtils.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#include <unistd.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

namespace util
{

    fs::path GetExecutablePath()
    {
#ifdef _WIN32
        char buffer[MAX_PATH] = {0};
        DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return fs::path(std::string(buffer, len));

#elif __APPLE__
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        std::string buffer(size, '\0');
        _NSGetExecutablePath(buffer.data(), &size);

        return fs::canonical(buffer);

#else // Linux
        char buffer[PATH_MAX] = {0};
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        return fs::path(std::string(buffer, len));
#endif
    }

    fs::path GetExecutableDir()
    {
        return GetExecutablePath().parent_path();
    }

} // namespace util
