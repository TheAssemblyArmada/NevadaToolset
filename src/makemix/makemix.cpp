#include "always.h"
#include "crc.h"
#include "ini.h"
#include "mixcreate.h"
#include "mpmath.h"
#include "pk.h"
#include "ramfile.h"
#include "rawfile.h"
#include "readline.h"
#include "rndstraw.h"

#ifdef PLATFORM_WINDOWS
#include <sysinfoapi.h>
#endif

void Print_Help(void)
{
    printf("\nMakeMix v0.1 " __DATE__
           ".\n\n"
           "Usage is makemix [-e -s -c -q -i directory -m filename -h -?] mixfile.\n\n"
           "    -e Encrypt the file header, supported in Red Alert onward.\n\n"
           "    -s SHA1 hash the file body, supported in Red Alert onward.\n\n"
           "    -c Use CRC32 instead of C&C Hash, required for TS onward.\n\n"
           "    -q Quiet mode, don't print status.\n\n"
           "    -i Set the directory to look for files in.\n"
           "       This defaults to current working directory.\n\n"
           "    -m Specify a manifest file, listing the files to add.\n"
           "       Will add all files found in the search directory otherwise.\n\n"
           "    -h -? displays this help.\n\n");
}

bool Encrypt;
bool Checksum;
bool Quiet;
bool UseCRC;
const char *BasePath;

template<typename CRC>
int Create_From_Manifest(const char *manifest, const char *outfile)
{
    char buffer[512];
    char fullpath[PATH_MAX];
    bool eof = false;
    RawFileClass tfc(manifest);
    MixFileCreatorClass<RawFileClass, CRC> mc(outfile, Checksum, Encrypt, Quiet);
    tfc.Open();

    while (!eof) {
        Read_Line(tfc, buffer, sizeof(buffer), eof);

        if (strlen(buffer) > 0) {
            snprintf(fullpath, PATH_MAX, "%s/%s", BasePath, buffer);
            mc.Add_File(fullpath);
        }
    }

    tfc.Close();
    mc.Write_Mix();

    return 0;
}

template<typename CRC>
int Create_From_Directory(const char *outfile)
{
    MixFileCreatorClass<RawFileClass, CRC> mc(outfile, Checksum, Encrypt, Quiet);
    mc.Add_Files(BasePath);
    printf("Writing %s.\n", mc.Get_Filename());
    mc.Write_Mix();

    return 0;
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

void Key_Init(void)
{
    static char const Keys[] =
        "[PublicKey]\n1=AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V\n\n"
        "[PrivateKey]\n1=AigKVje8mROcR8QixnxUEF5b29Curkq01DNDWCdOG99XBqH79OaCiTCB\n\n";

    RAMFileClass file((void *)Keys, (int)strlen(Keys));

    INIClass ini;
    ini.Load(file);

    g_PublicKey = INI_Get_PKey(ini, true);
    g_PrivateKey = INI_Get_PKey(ini, false);
}

void Init_Random(void)
{
#ifdef PLATFORM_WINDOWS
    struct _SYSTEMTIME SystemTime;
#else // !PLATFORM_WINDOWS
    struct tm *SystemTime;
    struct timespec MicroTime;
#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_WINDOWS
    GetSystemTime(&SystemTime);
    g_CryptRandom.Seed_Byte((char)SystemTime.wMilliseconds);
    g_CryptRandom.Seed_Bit(SystemTime.wSecond);
    g_CryptRandom.Seed_Bit(SystemTime.wSecond >> 1);
    g_CryptRandom.Seed_Bit(SystemTime.wSecond >> 2);
    g_CryptRandom.Seed_Bit(SystemTime.wSecond >> 3);
    g_CryptRandom.Seed_Bit(SystemTime.wSecond >> 4);
    g_CryptRandom.Seed_Bit(SystemTime.wMinute);
    g_CryptRandom.Seed_Bit(SystemTime.wMinute >> 1);
    g_CryptRandom.Seed_Bit(SystemTime.wMinute >> 2);
    g_CryptRandom.Seed_Bit(SystemTime.wMinute >> 3);
    g_CryptRandom.Seed_Bit(SystemTime.wMinute >> 4);
    g_CryptRandom.Seed_Bit(SystemTime.wHour);
    g_CryptRandom.Seed_Bit(SystemTime.wDay);
    g_CryptRandom.Seed_Bit(SystemTime.wDayOfWeek);
    g_CryptRandom.Seed_Bit(SystemTime.wMonth);
    g_CryptRandom.Seed_Bit(SystemTime.wYear);
#else // !PLATFORM_WINDOWS
    MiscUtil::Clock_Get_Time(CLOCK_REALTIME, &MicroTime);
    SystemTime = localtime(&MicroTime.tv_sec);
    g_CryptRandom.Seed_Byte(MicroTime.tv_nsec / 1000);
    g_CryptRandom.Seed_Bit(SystemTime->tm_sec);
    g_CryptRandom.Seed_Bit(SystemTime->tm_sec >> 1);
    g_CryptRandom.Seed_Bit(SystemTime->tm_sec >> 2);
    g_CryptRandom.Seed_Bit(SystemTime->tm_sec >> 3);
    g_CryptRandom.Seed_Bit(SystemTime->tm_sec >> 4);
    g_CryptRandom.Seed_Bit(SystemTime->tm_min);
    g_CryptRandom.Seed_Bit(SystemTime->tm_min >> 1);
    g_CryptRandom.Seed_Bit(SystemTime->tm_min >> 2);
    g_CryptRandom.Seed_Bit(SystemTime->tm_min >> 3);
    g_CryptRandom.Seed_Bit(SystemTime->tm_min >> 4);
    g_CryptRandom.Seed_Bit(SystemTime->tm_hour);
    g_CryptRandom.Seed_Bit(SystemTime->tm_mday);
    g_CryptRandom.Seed_Bit(SystemTime->tm_wday);
    g_CryptRandom.Seed_Bit(SystemTime->tm_mon);
    g_CryptRandom.Seed_Bit(SystemTime->tm_year);
#endif // PLATFORM_WINDOWS
}

int main(int argc, char *argv[])
{
    BasePath = "./";
    char const *manifest = NULL;

    if (argc <= 1) {
        Print_Help();
        return 0;
    }

    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-e") == 0) {
            Encrypt = true;
            Init_Random();
            Key_Init();
        } else if (strcmp(argv[i], "-s") == 0) {
            Checksum = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            UseCRC = true;
        } else if (strcmp(argv[i], "-q") == 0) {
            Quiet = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            BasePath = argv[++i];
        } else if (strcmp(argv[i], "-m") == 0) {
            manifest = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0) {
            Print_Help();
            return 0;
        } else {
            break;
        }
    }

    if (i != argc - 1) {
        printf("You must specify a file name for your Mix file as the last parameter.");
        return 1;
    }

    if (UseCRC) {
#if 0 // TODO: TS/RA2 support
        if (manifest != NULL) {
            if (!Quiet) {
                printf("Creating CRC32 Mix file from mainifest %s.\n", manifest);
            }

            return Create_From_Manifest<CRC32Engine>(manifest, argv[argc - 1]);
        } else {
            if (!Quiet) {
                printf("Creating CRC32 Mix file from directory %s.\n", BasePath);
            }

            return Create_From_Directory<CRC32Engine>(argv[argc - 1]);
        }
#endif
    } else {
        if (manifest != NULL) {
            if (!Quiet) {
                printf("Creating C&C Hash Mix file from mainifest %s.\n", manifest);
            }

            return Create_From_Manifest<CRCEngine>(manifest, argv[argc - 1]);
        } else {
            if (!Quiet) {
                printf("Creating C&C Hash Mix file from directory %s.\n", BasePath);
            }

            return Create_From_Directory<CRCEngine>(argv[argc - 1]);
        }
    }

    return 0;
}