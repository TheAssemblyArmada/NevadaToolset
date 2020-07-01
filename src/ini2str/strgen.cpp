/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Command line front end for string table conversion.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "always.h"
#include "ini2str.h"
#include "win32compat.h"

int main(int argc, char **argv)
{
    static const char table_name[] = "conquer.eng";
    static const char ini_name[] = "strings.ini";

    Handle_Win32_Args(&argc, &argv);

    bool pack = false;
    const char *in = nullptr;
    const char *out = nullptr;
    const char *lang = "eng";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            pack = true;
        }

        if (strcmp(argv[i], "-i") == 0) {
            in = argv[i + 1];
        }

        if (strcmp(argv[i], "-o") == 0) {
            out = argv[i + 1];
        }

        // Add support for additional encodings by altering the table in StrINIClass::Get_Lang_Code in strini.cpp
        if (strcmp(argv[i], "-l") == 0) {
            lang = argv[i + 1];
        }

        if (strcmp(argv[i], "-b") == 0) {
            g_LineBreak = argv[i + 1][0];
        }

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0) {
            printf(
                "strgen.\n\nUsage is strgen [-p -b linebreak -l language"
                "-i filename -o filename].\n\n"
                "    -p tells Stringset to pack strings from an ini file to a string file.\n"
                "       The default is to unpack a string file to an ini file.\n\n"
                "    -i filename sets the input file name to look for.\n\n"
                "    -o filename sets the output file name.\n\n"
                "    -b sets the character to treat as a line break replacement.\n"
                "       Defaults to '`'.\n\n"
                "    -l xxx three character language code to flag which character\n"
                "       encoding to use.\n\n"
                "    -h -? displays this help.\n\n");

            return 0;
        }
    }

    if (pack) {
        if (in == nullptr) {
            in = ini_name;
        }

        if (out == nullptr) {
            out = table_name;
        }

        return INI_To_StringTable(in, out, lang);

    } else {
        if (in == nullptr) {
            in = table_name;
        }

        if (out == nullptr) {
            out = ini_name;
        }

        return StringTable_To_INI(in, out, lang);
    }
}
