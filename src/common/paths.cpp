/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Functions for providing platform specific paths for data.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "paths.h"
#include "whereami.h"

const std::string &Paths::Program_Name()
{
    static std::string _path;

    if (_path.empty()) {
        int length = wai_getExecutablePath(nullptr, 0, nullptr);
        std::string tmp;
        int dirname_length = -1;

        if (length >= 0) {
            tmp.resize(size_t(length) + 1);
            wai_getExecutablePath(&tmp[0], length, &dirname_length);
        }

        _path = tmp.substr(dirname_length + 1);
    }

    return _path;
}

const std::string &Paths::Program_Path()
{
    static std::string _path;

    if (_path.empty()) {
        int length = wai_getExecutablePath(nullptr, 0, nullptr);
        std::string tmp;
        int dirname_length = -1;

        if (length >= 0) {
            tmp.resize(size_t(length) + 1);
            wai_getExecutablePath(&tmp[0], length, &dirname_length);
        }

        _path = tmp.substr(0, dirname_length);
    }

    return _path;
}
