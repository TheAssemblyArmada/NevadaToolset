/**
 * @file
 *
 * @author CCHyper
 * @author OmniBlade
 *
 * @brief INI file parsing class customized for string table file compiling.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "ini.h"

#define STRINI_MAX_BUF_SIZE 4096

class StrINIClass : public INIClass
{
    struct LangMap
    {
        const char *lang;
        int code;
    };

public:
    StrINIClass() {}
    virtual ~StrINIClass() {}

    virtual int Load(Straw &straw) override;

    bool Put_Table_String(const char *section, const char *entry, const char *string, const char *lang = nullptr);
    int Get_Table_String(const char *section, const char *entry, const char *defvalue = "", char *buffer = nullptr,
        int length = 0, const char *lang = nullptr) const;

private:
    static int Get_Lang_Code(const char *lang);
    static char *Codepage_To_UTF8(const char *string, int codepage);
    static char *UTF8_To_Codepage(const char *string, int codepage);
};
