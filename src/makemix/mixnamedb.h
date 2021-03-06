/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Class for handling CRC to string lookups.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#ifndef MIXNAMEDB_H
#define MIXNAMEDB_H

#include <map>
#include <string>

class FileClass;

class MixNameDatabase
{
public:
    enum HashMethod
    {
        HASH_RACRC,
        HASH_CRC32,
        HASH_COUNT,
        HASH_ANY = HASH_COUNT,
    };

    struct NameEntry
    {
        std::string file_name;
        std::string file_desc;
    };

    struct DataEntry
    {
        std::string file_name;
        std::string file_desc;
        int32_t ra_crc;
        int32_t ts_crc;
    };

    ~MixNameDatabase() { Save_To_Ini(); } // Write the file to the configured file name.

    void Read_From_XCC(const char *dbfile);
    bool Read_From_Ini(const char *ini_file);
    void Read_Internal();
    const NameEntry &Get_Entry(int32_t hash, HashMethod crc_type = HASH_ANY);
    bool Add_Entry(const char *file_name, const char *comment, HashMethod crc_type = HASH_ANY);
    void Save_To_Yaml(const char *yaml_file = nullptr);
    void Save_To_Ini(const char *ini_file = nullptr);

private:
    void Read_XCC_Tranch(FileClass &fc, HashMethod crc_type);
    void Regenerate_Hash_Maps();
    static void Get_String(FileClass &fc, std::string &dst);

private:
    std::map<int, NameEntry> m_hashMaps[HASH_COUNT];
    std::map<std::string, DataEntry> m_nameMap;
    std::string m_saveName;
    bool m_isMapDirty;
};

#endif
