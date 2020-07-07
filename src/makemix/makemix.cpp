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
#include <gitverinfo.h>

#ifdef PLATFORM_WINDOWS
#include <sysinfoapi.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

void Print_Help()
{
    printf("\nMakeMix Revision: %s Date: %s"
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
           "    -h -? displays this help.\n\n",
        g_gitShortSHA1,
        g_gitCommitDate);
}

bool g_encrypt;
bool g_checksum;
bool g_quiet;
bool g_useCRC;
const char *g_basePath;

template<typename CRC>
int Create_From_Manifest(const char *manifest, const char *outfile)
{
    char buffer[512];
    char fullpath[PATH_MAX];
    bool eof = false;
    RawFileClass tfc(manifest);
    MixFileCreatorClass<RawFileClass, CRC> mc(outfile, g_checksum, g_encrypt, g_quiet);
    tfc.Open();

    while (!eof) {
        Read_Line(tfc, buffer, sizeof(buffer), eof);

        if (strlen(buffer) > 0) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", g_basePath, buffer);
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
    MixFileCreatorClass<RawFileClass, CRC> mc(outfile, g_checksum, g_encrypt, g_quiet);
    mc.Add_Files(g_basePath);
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

void Init_Random()
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
#elif defined HAVE_SYS_TIME_H
    struct tm *sys_time;
    struct timeval curr_time;
    gettimeofday(&curr_time, nullptr);
    sys_time = localtime(&curr_time.tv_sec);

    g_CryptRandom.Seed_Byte(curr_time.tv_usec / 1000);
    g_CryptRandom.Seed_Bit(sys_time->tm_sec);
    g_CryptRandom.Seed_Bit(sys_time->tm_sec >> 1);
    g_CryptRandom.Seed_Bit(sys_time->tm_sec >> 2);
    g_CryptRandom.Seed_Bit(sys_time->tm_sec >> 3);
    g_CryptRandom.Seed_Bit(sys_time->tm_sec >> 4);
    g_CryptRandom.Seed_Bit(sys_time->tm_min);
    g_CryptRandom.Seed_Bit(sys_time->tm_min >> 1);
    g_CryptRandom.Seed_Bit(sys_time->tm_min >> 2);
    g_CryptRandom.Seed_Bit(sys_time->tm_min >> 3);
    g_CryptRandom.Seed_Bit(sys_time->tm_min >> 4);
    g_CryptRandom.Seed_Bit(sys_time->tm_hour);
    g_CryptRandom.Seed_Bit(sys_time->tm_mday);
    g_CryptRandom.Seed_Bit(sys_time->tm_wday);
    g_CryptRandom.Seed_Bit(sys_time->tm_mon);
    g_CryptRandom.Seed_Bit(sys_time->tm_year);
#else
#error Suitable time functions not found.
#endif // PLATFORM_WINDOWS
}

int main(int argc, char *argv[])
{
    g_basePath = "./";
    const char *manifest = NULL;

    if (argc <= 1) {
        Print_Help();
        return 0;
    }

    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-e") == 0) {
            g_encrypt = true;
            Init_Random();
            Key_Init();
        } else if (strcmp(argv[i], "-s") == 0) {
            g_checksum = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            g_useCRC = true;
        } else if (strcmp(argv[i], "-q") == 0) {
            g_quiet = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            g_basePath = argv[++i];
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

    if (g_useCRC) {
        if (manifest != NULL) {
            if (!g_quiet) {
                printf("Creating CRC32 Mix file from mainifest %s.\n", manifest);
            }

            return Create_From_Manifest<CRC32Engine>(manifest, argv[argc - 1]);
        } else {
            if (!g_quiet) {
                printf("Creating CRC32 Mix file from directory %s.\n", g_basePath);
            }

            return Create_From_Directory<CRC32Engine>(argv[argc - 1]);
        }
    } else {
        if (manifest != NULL) {
            if (!g_quiet) {
                printf("Creating C&C Hash Mix file from mainifest %s.\n", manifest);
            }

            return Create_From_Manifest<CRCEngine>(manifest, argv[argc - 1]);
        } else {
            if (!g_quiet) {
                printf("Creating C&C Hash Mix file from directory %s.\n", g_basePath);
            }

            return Create_From_Directory<CRCEngine>(argv[argc - 1]);
        }
    }

    return 0;
}