#include "paths.h"
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

const std::string &Paths::Data_Path()
{
    static std::string _path;

    if (_path.empty()) {
        // TODO, this should be something like INSTALL_PREFIX/share/nevadatools or some such.
        _path = Program_Path();
    }

    return _path;
}

const std::string &Paths::User_Path()
{
    static std::string _path;

    if (_path.empty()) {
        char *tmp = getenv("HOME");

        if (tmp == nullptr) {
            tmp = getpwuid(getuid())->pw_dir;
        }

        if (tmp != nullptr) {
            _path = tmp;
#ifdef PLATFORM_OSX
            _path += "/.config";
            mkdir(_path.c_str(), 0700);
            _path += "/nevadatools";
#else
            _path += "/Library/Application Support/NevadaTools";
#endif
            mkdir(_path.c_str(), 0700);
        }
    }

    return _path;
}