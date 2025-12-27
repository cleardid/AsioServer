
#ifndef STRING_FORMAT_H
#define STRING_FORMAT_H

#pragma region 字符串格式转换

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <stringapiset.h>

std::string ConvertStringToUTF8(const std::string &str)
{
    int wlen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wlen == 0)
        return str;

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], wlen);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0)
        return str;

    std::string utf8_str(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);

    // 去除末尾的 \0
    if (!utf8_str.empty() && utf8_str.back() == '\0')
    {
        utf8_str.pop_back();
    }

    return utf8_str;
}
#else
// Linux/Mac 版本，通常不需要转换
std::string ConvertStringToUTF8(const std::string &str)
{
    return str;
}
#endif

#pragma endregion

#endif // STRING_FORMAT_H