/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Command line mix extracting interface.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "gitverinfo.h"
#include "ini.h"
#include "mixfile.h"
#include "mixnamedb.h"
#include "paths.h"
#include "pk.h"
#include "ramfile.h"
#include "rawfile.h"
#include <string>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include <sysinfoapi.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

bool quiet = true;

void Print_Help()
{
    printf(
        "\nUnmakeMix Revision: %s Date: %s"
        ".\n\n"
        "Usage is unmakemix [-x -l -c -q -o directory -f filename -h -?] mixfile.\n\n"
        "    -x Extract files from the mixfile.\n\n"
        "    -l List the contents of the mixfile.\n\n"
        "    -c Force use of CRC32 instead of C&C Hash if you know you have a TS mix.\n\n"
        "    -v Verbose mode, print current status.\n\n"
        "    -o Set the directory to output files to.\n"
        "       This defaults to current working directory.\n\n"
        "    -f Specify a file to extract specifically.\n"
        "       Can specify this option multiple times for multiple files.\n"
        "       Will extract all files found in the mix otherwise.\n\n"
        "    -h -? displays this help.\n\n",
        g_gitShortSHA1,
        g_gitCommitDate);
}

PKey INI_Get_PKey(INIClass &ini, bool fast)
{
    PKey key;
    uint8_t buffer[512];

    if (fast) {
        captainslog_debug("INI_Get_PKey() - Preparing PublicKey...\n");
        Int<MAX_UNIT_PRECISION> exp(0x10001);
        MPMath::DER_Encode(exp, buffer, MAX_UNIT_PRECISION);
    } else {
        captainslog_debug("INI_Get_PKey() - Loading PrivateKey\n");
        ini.Get_UUBlock("PrivateKey", buffer, sizeof(buffer));
    }

    captainslog_debug("INI_Get_PKey() - Decoding Exponent\n");
    key.Decode_Exponent(buffer);

    captainslog_debug("INI_Get_PKey() - Loading PublicKey\n");

    ini.Get_UUBlock("PublicKey", buffer, sizeof(buffer));

    captainslog_debug("INI_Get_PKey() - Decoding Modulus\n");
    key.Decode_Modulus(buffer);

    return key;
}

void Key_Init()
{
    static const char Keys[] =
        "[PublicKey]\n1=AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V\n\n"
        "[PrivateKey]\n1=AigKVje8mROcR8QixnxUEF5b29Curkq01DNDWCdOG99XBqH79OaCiTCB\n\n";

    RAMFileClass file((void *)Keys, (int)strlen(Keys));

    INIClass ini;
    ini.Load(file);

    g_PublicKey = INI_Get_PKey(ini, true);
    g_PrivateKey = INI_Get_PKey(ini, false);
}

template<typename I>
std::string Int_To_Hex(I w, size_t hex_len = sizeof(I) << 1)
{
    static const char *digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');

    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
        rc[i] = digits[(w >> j) & 0x0f];
    }

    return rc;
}

template<typename CRC>
void List_Mix(const char *filename, MixNameDatabase &name_db, MixNameDatabase::HashMethod hash)
{
    MixFileClass<RawFileClass, CRC> mixfile(filename, &g_PublicKey);
    const FileInfoStruct *index = mixfile.Get_Index();
    printf("%-24s  %-10s  %-10s\n", "Filename", "Offset", "Size");
    printf("%-24s  %10s  %10s\n", "========================", "==========", "==========");

    for (int i = 0; i < mixfile.Get_File_Count(); ++i) {
        std::string filename = name_db.Get_Entry(index[i].m_CRC, hash).file_name;

        if (filename.empty()) {
            filename = Int_To_Hex(index[i].m_CRC);
        }

        printf("%-24s  %*d  %*d\n",
            filename.c_str(),
            10,
            index[i].m_Offset,
            10,
            index[i].m_Size);
    }
}

template<typename CRC>
void Extract_Mix(
    const char *filename, const char *base_path, MixNameDatabase &name_db, MixNameDatabase::HashMethod hash, const std::vector<std::string> &files)
{
    const size_t BUFFER_SIZE = 2048;
    MixFileClass<RawFileClass, CRC> mixfile(filename, &g_PublicKey);

    if (files.empty()) {
        const FileInfoStruct *index = mixfile.Get_Index();
        RawFileClass reader(filename);
        
        if (!reader.Open(FM_READ)) {
            return;
        }

        // Iterate the mix file index and extract the files found.
        for (int i = 0; i < mixfile.Get_File_Count(); ++i) {
            uint8_t buffer[BUFFER_SIZE];
            std::string filename;

            if (base_path != nullptr && *base_path != '\0') {
                filename = base_path;
                filename += "/";
            }
            
            if ((hash == MixNameDatabase::HASH_RACRC && (uint32_t)index[i].m_CRC == 0x54C2D545)
                || (hash == MixNameDatabase::HASH_CRC32 && (uint32_t)index[i].m_CRC == 0x366E051F)) {
                if (!quiet) {
                    printf("XCC local mix database extension detected, skipping entry.\n.");
                }

                continue;
            }

            // If we don't get a useable filename back, then use the hex version of the file CRC to create a filename.
            if (name_db.Get_Entry(index[i].m_CRC, hash).file_name.empty()) {
                filename += Int_To_Hex(index[i].m_CRC);
            } else {
                filename += name_db.Get_Entry(index[i].m_CRC, hash).file_name;
            }

            int offset;
            int size;
            MixFileClass<RawFileClass, CRC> *mp;

            if (!mixfile.Offset(index[i].m_CRC, nullptr, &mp, &offset, &size)) {
                if (!quiet) {
                    printf("Failed to find '0x%08X' in any loaded mix file.\n", index[i].m_CRC);
                }

                continue;
            }

            if (offset < 0 || size < 0) {
                if (!quiet) {
                    printf("A corrupt index entry was found, ignoring faulty file.");
                }

                continue;
            }

            RawFileClass writer(filename.c_str());
            
            // If file won't open for writing then try next file.
            if (!writer.Open(FM_WRITE)) {
                if (!quiet) {
                    printf("Couldn't open '%s' for writing.\n", writer.File_Name());
                }

                continue;
            }
            
            if (!quiet) {
                printf("Extracting '%s'.\n", writer.File_Name());
            }

            // Seek to the file in question and start copying its contents.
            reader.Seek(offset, FS_SEEK_START);

            while (size > BUFFER_SIZE) {
                // Make sure we actually read the amount of data we expected to.
                if (reader.Read(buffer, BUFFER_SIZE) == BUFFER_SIZE) {
                    writer.Write(buffer, BUFFER_SIZE);
                    size -= BUFFER_SIZE;
                } else  {
                    if (!quiet) {
                        printf("Failed to read sufficient data.\n");
                    }

                    break;
                }
            }

            if (size > 0 && size < BUFFER_SIZE) {
                if (reader.Read(buffer, size) == size) {
                    writer.Write(buffer, size);
                } else if (!quiet) {
                    printf("Failed to read sufficient data.\n");
                }
            }
        }
    } else {
        for (auto it = files.begin(); it != files.end(); ++it) {
            int offset;
            int size;
            MixFileClass<RawFileClass, CRC> *mp; 
            
            if (!mixfile.Offset(it->c_str(), nullptr, &mp, &offset, &size)) {
                if (!quiet) {
                    printf("Failed to find '%s' in any loaded mix file.\n", it->c_str());
                }

                continue;
            }

            RawFileClass reader(mp->Get_Filename());

            if (!reader.Open(FM_READ)) {
                continue;
            }

            uint8_t buffer[BUFFER_SIZE];
            std::string filename;

            if (base_path != nullptr && *base_path != '\0') {
                filename = base_path;
                filename += "/";
            }

            filename += *it;

            RawFileClass writer(filename.c_str());

            // If file won't open for writing then try next file.
            if (!writer.Open(FM_WRITE)) {
                if (!quiet) {
                    printf("Couldn't open '%s' for writing.\n", writer.File_Name());
                }

                continue;
            }

            // Seek to the file in question and start copying its contents.
            reader.Seek(offset, FS_SEEK_START);

            while (size > BUFFER_SIZE) {
                reader.Read(buffer, BUFFER_SIZE);
                writer.Write(buffer, BUFFER_SIZE);
                size -= BUFFER_SIZE;
            }

            if (size > 0) {
                reader.Read(buffer, size);
                writer.Write(buffer, size);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    bool extracting = false;
    bool listing = false;
    bool use_crc = false;
    const char *base_path = "";
    std::vector<std::string> files;

    if (argc <= 1) {
        Print_Help();
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-x") == 0) {
            extracting = true;
        } else if (strcmp(argv[i], "-l") == 0) {
            listing = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            use_crc = true;
        } else if (strcmp(argv[i], "-v") == 0) {
            quiet = false;
        } else if (strcmp(argv[i], "-o") == 0) {
            base_path = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0) {
            files.push_back(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0) {
            Print_Help();
            return 0;
        } else {
            break;
        }
    }

    // Load the local mix name database and if one doesn't exist, start one.
    MixNameDatabase namedb;
    bool prog_dir = false;
    std::string user_db = Paths::User_Path() + "/filenames.db";
    std::string prog_db = Paths::Program_Path() + "/filenames.db";
    if (!namedb.Read_From_Ini(user_db.c_str())) {
        if (!namedb.Read_From_Ini(prog_db.c_str())) {
            // Saving the internal to the user directory by default so it will auto save any changes later on exit.
            if (!quiet) {
                printf("Failed to read file name database generating from internal db.\n");
            }
            namedb.Read_Internal();
            namedb.Save_To_Ini(user_db.c_str());
        } else {
            prog_dir = true;
        }
    }

    // Load any file names we got on the command line into the database.
    for (auto it = files.begin(); it != files.end(); ++it) {
        namedb.Add_Entry(it->c_str(), "");
    }

    // We are about to start actually doing stuff on mix files, initialise the decryption keys.
    Key_Init();

    // If we want a list of files in each mix, print them out here.
    if (listing) {
        if (use_crc) {
            List_Mix<CRC32Engine>(argv[argc - 1], namedb, MixNameDatabase::HashMethod::HASH_CRC32);
        } else {
            List_Mix<CRCEngine>(argv[argc - 1], namedb, MixNameDatabase::HashMethod::HASH_RACRC);
        }
    }

    if (extracting) {
        if (use_crc) {
            Extract_Mix<CRC32Engine>(argv[argc - 1], base_path, namedb, MixNameDatabase::HashMethod::HASH_CRC32, files);
        } else {
            Extract_Mix<CRCEngine>(argv[argc - 1], base_path, namedb, MixNameDatabase::HashMethod::HASH_RACRC, files);
        }
    }

    return 0;
}
