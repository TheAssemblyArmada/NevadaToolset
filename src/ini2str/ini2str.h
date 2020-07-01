/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Conversion functions for string tables to uft8 ini files.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

extern char g_LineBreak;

int INI_To_StringTable(const char *ini_name, const char *table_name, const char *lang);
int StringTable_To_INI(const char *table_name, const char *ini_name, const char *lang);
