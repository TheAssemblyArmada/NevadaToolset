/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Class for providing platform specific paths for data.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#ifndef PATHS_H
#define PATHS_H

#include "always.h"
#include <string>

namespace Paths
{
    const std::string &Program_Name();
    const std::string &Program_Path();
    const std::string &User_Path();
    const std::string &Data_Path();
};

#endif
