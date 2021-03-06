/**
 * @file
 *
 * @author CCHyper
 * @author OmniBlade
 *
 * @brief INI file parsing class.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#ifndef INI_H
#define INI_H

#include "always.h"
#include "crc.h"
#include "index.h"
#include "listnode.h"
#include "pipe.h"
#include "straw.h"
#include <string.h>

#define SYS_NEW_LINE "\r\n"

class FileClass;
class Straw;
class Pipe;

// find out what this really is.
enum INIIntegerType
{
    INIINTEGER_AS_DECIMAL = 0,
    INIINTEGER_AS_HEX = 1,
    INIINTEGER_AS_MOTOROLA_HEX = 2
};

enum INILoadType
{
    INI_LOAD_INVALID = 0,
    INI_LOAD_OVERWRITE = 1,
    INI_LOAD_CREATE_OVERRIDES = 2
};

class INIEntry : public Node<INIEntry *>
{
public:
    // Class constructor and deconstructor.
    INIEntry(const char *name, const char *value) : m_key(strdup(name)), m_value(strdup(value)) {}
    ~INIEntry();

    // int Index_ID();    //what could this be returning?

    const char *Get_Name() { return m_key; }
    void Set_Name(const char *name);

    const char *Get_Value() { return m_value; }
    void Set_Value(const char *new_value);

    int32_t const CRC(const char *value) const { return Calculate_CRC<CRC32Engine>(value, (int)strlen(value)); }

public:
    char *m_key;
    char *m_value;
};

class INISection : public Node<INISection *>
{
public:
    INISection(const char *name) : m_sectionName(strdup(name)){}
    ~INISection();

    // int Index_ID();    //what could this be returning? 

    INIEntry *Find_Entry(const char *entry) const;

    const char *Get_Name() const { return m_sectionName; }
    void Set_Name(const char *str);

    int Get_Entry_Count() const { return m_entryIndex.Count(); }

    int32_t const CRC(const char *value) const { return Calculate_CRC<CRC32Engine>(value, unsigned(strlen(value))); }

public:
    char *m_sectionName;
    List<INIEntry *> m_entryList;
    IndexClass<int, INIEntry *> m_entryIndex;
};

/**
 * @brief Implements INI file handling, everything is handled in memory once loaded.
 * 
 *    Here are example file contents:
 * 
 *      [TankObject]
 *      Name = Super Awesome Tank
 *      MaxCount = 1
 *      Enabled = true
 *      ; Comment string
 * 
 *      [General]
 *      MaxTankCount = 999
 * 
 *    The "[TankObject]" part is a section called "TankObject".
 *    The "Name" part is a key called "Name".
 *    The "Super Awesome Tank" part is a value called "Super Awesome Tank".
 *    The "; Comment string" part is a comment line that has no effect.
 * 
 *    Some format specification details regarding this implementation:
 *        - Leading and trailing space is removed from section, key, and value strings returned to the user.
 *        - If you want to preserve leading and trailing space, surround the text with quote characters.
 *        - Spaces are valid in section, key, and value strings.
 *        - The only reserved characters are the [] characters in the section string and the = char in the key string.
 *        - The key = section sequence need not have spaces between the key and = and the = and section.
 *        - Section and key strings are case sensitive when the user is doing reads.
 *        - Newlines can be in either Unix format ("\n") or Windows format ("\r\n").
 *        - When writing an ini, the new lines are always windows format for consistency with the original.
 *        - Supported text encodings for .ini files include ASCII, UTF8 Unicode.
 * 
 *    Example usage:
 *        INIClass INIClass("somefile.ini");
 * 
 *        int rating;
 *        if(iniFile.ReadEntryFormatted("User", "Rating", "%d", &rating) > 0)
 *            DEBUG_SAY("User rating is %d.", rating);
 * 
 *        BoardBitmapData bbd;
 *        if(iniFile.ReadBinary("Board", "Bitmap", &bbd, sizeof(bbd));
 *            DEBUG_SAY("Board bitmap binary data read successfully.");
 * 
 */
class INIClass
{
public:
    enum
    {
        MAX_LINE_LENGTH = 128, // this is 512 in Generals, 4096 in BFME
        MAX_BUF_SIZE = 4096,
        MAX_UUBLOCK_LINE_LENGTH = 70,
        MAX_TEXTBLOCK_LINE_LENGTH = 75
    };

public:
    INIClass() {}
    virtual ~INIClass();

    bool Clear(const char *section = nullptr, const char *entry = nullptr);
    bool Is_Loaded() const { return m_sectionList.First()->Is_Valid(); }

    int Save(const char *filename) const;
    int Save(FileClass &file) const;
    virtual int Save(Pipe &pipe) const;
    int Load(const char *filename);
    int Load(FileClass &file);
    virtual int Load(Straw &straw);

    List<INISection *> &Get_Section_List() { return m_sectionList; }
    IndexClass<int, INISection *> &Get_Section_Index() { return m_sectionIndex; }

    bool Is_Present(const char *section, const char *entry = nullptr);

    // Returns the section object if it exists.
    INISection *Find_Section(const char *section) const;
    bool Section_Present(const char *section) const { return Find_Section(section) != nullptr; }
    int Section_Count() { return m_sectionIndex.Count(); }

    // Returns the entry object if it exists.
    INIEntry *Find_Entry(const char *section, const char *entry) const;
    int Entry_Count(const char *section) const;
    const char *Get_Entry(const char *section, int index) const;

    // Enumerate_Entries()
    //   Enumerates all entries (key/value pairs) of a given section.
    //   Returns the number of entries present or -1 upon error.
    int Enumerate_Entries(const char *section, const char *entry_prefix, uint32_t start_number, uint32_t end_number);

    bool Put_UUBlock(const char *section, void *block, int length);
    int Get_UUBlock(const char *section, void *block, int length = 0) const;
    bool Put_TextBlock(const char *section, const char *text);
    int Get_TextBlock(const char *section, char *block, int length = 0) const;
    bool Put_Int(const char *section, const char *entry, int value, int format = INIINTEGER_AS_DECIMAL);
    int Get_Int(const char *section, const char *entry, int defvalue = 0) const;
    bool Put_Bool(const char *section, const char *entry, bool value);
    bool const Get_Bool(const char *section, const char *entry, bool defvalue = false) const;
    bool Put_Hex(const char *section, const char *entry, int value);
    int Get_Hex(const char *section, const char *entry, int defvalue = 0) const;
    bool Put_Float(const char *section, const char *entry, double value);
    float Get_Float(const char *section, const char *entry, float defvalue = 0) const;
    bool Put_Double(const char *section, const char *entry, double value);
    double Get_Double(const char *section, const char *entry, double defvalue = 0) const;
    bool Put_String(const char *section, const char *entry, const char *string);
    int Get_String(const char *section, const char *entry, const char *defvalue = "", char *buffer = nullptr,
        int length = 0) const;

#ifdef GAME_DLL
    int Hook_Load(Straw &straw) { return INIClass::Load(straw); }
#endif
protected:
    static void Strip_Comments(char *line);
    static int32_t const CRC(const char *string);

protected:
    List<INISection *> m_sectionList;
    IndexClass<int, INISection *> m_sectionIndex;
};

#endif // INI_H
