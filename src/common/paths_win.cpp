#include "paths.h"
#include "utf.h"
#include <fileapi.h>

const std::string &Paths::Data_Path()
{
    static std::string _path;

    if (_path.empty()) {
        _path = Program_Path();
    }

    return _path;
}

const std::string &Paths::User_Path()
{
    static std::string _path;

    if (_path.empty()) {
        wchar_t *tmp = _wgetenv(L"LOCALAPPDATA");

        if (tmp == nullptr || tmp[0] == '\0') {
            wchar_t *tmp = _wgetenv(L"APPDATA");
        }

        if (tmp != nullptr) {
            _path = UTF16To8(tmp);
            _path += "\\NevadaTools";
            CreateDirectoryW(UTF8To16(_path.c_str()), nullptr);
        } else {
            _path = Program_Path();
        }
    }

    return _path;
}
