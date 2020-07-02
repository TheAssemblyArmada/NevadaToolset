#pragma once

#include "always.h"
#include "crc.h"
#include "endiantype.h"
#include "filepipe.h"
#include "index.h"
#include "listnode.h"
#include "mixfile.h"
#include "pk.h"
#include "pkpipe.h"
#include "shapipe.h"
#include <captainslog.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <winbase.h>
#else
// Needed for basename
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // !PLATFORM_WINDOWS

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

template<class FC, typename CRC>
class MixFileCreatorClass
{
private:
    class FileDataNode : public Node<FileDataNode *>
    {
    public:
        FileDataNode(void) : Node<FileDataNode *>(), Entry(), FilePath(nullptr) {}
        ~FileDataNode(void);

    public:
        FileInfoStruct Entry;
        char *FilePath;
    };

public:
    MixFileCreatorClass(char const *filename, bool has_checksum = false, bool is_encrypted = false, bool quiet = true,
        bool force_flags = false);
    ~MixFileCreatorClass(void);

    char const *Get_Filename(void) const
    {
        char const *name = MixFile->File_Name();
        if (name == nullptr) {
            return TempFilename;
        }
        return name;
    }

    char const *Get_Temp_Filename(void) const { return TempFilename; };
    void Set_Filename(char const *filename) { MixFile->Set_Name(filename); }
    void Add_File(char const *filename);
    void Add_Files(char const *path);
    void Remove_File(char const *filename);
    void Write_Mix(void);

private:
    static char TempFilename[];

protected:
    List<FileDataNode *> FileList;
    IndexClass<int32_t, FileDataNode *> FileIndex;
    FC *MixFile;
    bool HasChecksum;
    bool IsEncrypted;
    bool ForceFlags;
    bool Quiet;
    MIXHeaderStruct Header;

private:
    int Write_File(char const *filename, Pipe *pipe);
};

template<class FC, typename CRC>
char MixFileCreatorClass<FC, CRC>::TempFilename[] = "MAKEMIX.MIX";

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::FileDataNode::~FileDataNode(void)
{
    if (FilePath) {
        free(FilePath);
    }
    FilePath = nullptr;
}

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::MixFileCreatorClass(
    char const *filename, bool has_checksum, bool is_encrypted, bool quiet, bool force_flags) :
    FileList(),
    FileIndex(),
    MixFile(nullptr),
    HasChecksum(has_checksum),
    IsEncrypted(is_encrypted),
    ForceFlags(force_flags),
    Quiet(quiet),
    Header()
{
    // Create a new file instance and store it.
    MixFile = filename != nullptr ? new FC(filename) : new FC(TempFilename);
}

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::~MixFileCreatorClass(void)
{
    if (MixFile) {
        delete MixFile;
    }

    MixFile = nullptr;
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Add_File(char const *filename)
{
    char path[PATH_MAX];
    path[PATH_MAX - 1] = '\0';

    FC file(filename);

    if (!file.Is_Available() || Header.file_count == USHRT_MAX || (Header.file_size + (uint64_t)file.Size()) > ULONG_MAX) {
        return;
    }

    strlcpy(path, filename, sizeof(path));
    FileDataNode *filedata = new FileDataNode;

    filedata->FilePath = strdup(path);

#ifdef PLATFORM_WINDOWS
    char basename[_MAX_FNAME];
    basename[_MAX_FNAME - 1] = '\0';

    char ext[_MAX_EXT];
    ext[_MAX_EXT - 1] = '\0';

    _splitpath(path, nullptr, nullptr, basename, ext);
    snprintf(path, PATH_MAX, "%s%s", basename, ext);

    filedata->Entry.m_CRC = Calculate_CRC<CRC>(strupr(path), (unsigned)strlen(path));
#else // !PLATFORM_WINDOWS
    char *basefname = basename(path); // basename can alter string so assign after duplicated
    filedata->Entry.m_CRC = Calculate_CRC<CRC>(strupr(basefname), (unsigned)strlen(basefname));
#endif // PLATFORM_WINDOWS

    filedata->Entry.m_Offset = Header.file_size;
    filedata->Entry.m_Size = file.Size();

    FileList.Add_Tail(filedata);
    FileIndex.Add_Index(filedata->Entry.m_CRC, filedata);

    filedata->Entry.m_CRC = htole32(filedata->Entry.m_CRC);
    filedata->Entry.m_Offset = htole32(filedata->Entry.m_Offset);
    filedata->Entry.m_Size = htole32(filedata->Entry.m_Size);

    Header.file_size += file.Size();
    ++Header.file_count;
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Add_Files(const char *path)
{
#ifdef PLATFORM_WINDOWS
    WIN32_FIND_DATA data;
    HANDLE handle;
    char fullpath[PATH_MAX];
    fullpath[PATH_MAX - 1] = '\0';

    snprintf(fullpath, PATH_MAX, "%s\\*", path);

    if ((handle = FindFirstFileA(fullpath, &data)) != INVALID_HANDLE_VALUE) {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                continue;
            }

            snprintf(fullpath, PATH_MAX, "%s/%s", path, data.cFileName);
            Add_File(fullpath);
        } while (FindNextFile(handle, &data) != false);

        FindClose(handle);
    }
#else
    DIR *dp;
    struct dirent *dirp;
    struct stat st;

    char fullpath[PATH_MAX];
    fullpath[PATH_MAX - 1] = '\0';

    if ((dp = opendir(path)) != nullptr) {
        while ((dirp = readdir(dp)) != nullptr) {
            snprintf(fullpath, PATH_MAX, "%s/%s", path, dirp->d_name);
            stat(fullpath, &st);

            if (S_ISDIR(st.st_mode)) {
                continue;
            }

            Add_File(fullpath);
        }

        closedir(dp);
    }
#endif
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Remove_File(const char *filename)
{
#ifdef PLATFORM_WINDOWS

#else
    char path[PATH_MAX];
    path[PATH_MAX - 1] = '\0';

    captainslog_assert(filename != nullptr);

    if (filename == nullptr || *filename == '\0') {
        return;
    }

    strlcpy(path, filename, sizeof(path));
    char *basefname = basename(path);
    uint32_t id = Calculate_CRC<CRC>(strupr(basefname), strlen(basefname));

    FileDataNode *filedata = FileIndex.Fetch_Index(id);

    if (!filedata) {
        return;
    }

    Header.file_size -= filedata->Entry.m_Size;
    --Header.file_count;
    filedata->Unlink();
    FileIndex.Remove_Index(id);

    delete filedata;
#endif
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Write_Mix(void)
{
    MIXHeaderStruct tmpheader;
    SHAEngine::SHADigest digest;

    PKPipe pkpipe(PIPE_ENCRYPT, g_CryptRandom);
    SHAPipe shpipe;
    FilePipe flpipe(MixFile);

    Pipe *pipe_to_use = &flpipe;

    int headersize = 0;
    int bodysize = 0;

    MixFile->Open(FM_WRITE);
    pkpipe.Put_To(&flpipe);
    shpipe.Put_To(&flpipe);

    // Write the flags and setup PKPipe if needed.
    if (IsEncrypted || HasChecksum || ForceFlags) {
        uint32_t flags = 0;

        if (IsEncrypted) {
            pkpipe.Key(&g_PrivateKey);
            pipe_to_use = &pkpipe;
            flags |= IS_ENCRYPTED;
        }

        if (HasChecksum) {
            flags |= HAS_CHECKSUM;
        }

        flags = htole32(flags);

        // Only use FilePipe to put the flags.
        headersize += flpipe.Put(&flags, sizeof(flags));
    }

    // Write the header, swapping byte order if needed.
    tmpheader.file_count = htole16(Header.file_count);
    tmpheader.file_size = htole32(Header.file_size);

    headersize += pipe_to_use->Put(&tmpheader, sizeof(tmpheader));

    // Write the index, the index should have been byte swapped as it was
    // generated if it was needed.
    for (int i = 0; i < FileIndex.Count(); ++i) {
        headersize += pipe_to_use->Put(&FileIndex.Fetch_Data_By_Index(i)->Entry, sizeof(FileInfoStruct));
    }

    headersize += pipe_to_use->Flush();

    // Change to either SHAPipe or plain FilePipe depending on if we need a
    // checksum.
    pipe_to_use = HasChecksum ? static_cast<Pipe *>(&shpipe) : static_cast<Pipe *>(&flpipe);

    // Loop through the file nodes and write the file data to the body of the
    // mix. These are written in the order they were added to the list, not the
    // sorted order that the index must be written in.
    for (FileDataNode *node = FileList.First(); node->Next() != nullptr; node = node->Next()) {
        if (!Quiet) {
            printf("Writing file %s\n", node->FilePath);
        }

        bodysize += Write_File(node->FilePath, pipe_to_use);
    }

    // Write the checksum if needed, this will have been generated during the
    // writing of the mix body if it was required.
    if (HasChecksum) {
        shpipe.Result(&digest);
        flpipe.Put(&digest, sizeof(digest));
    }

    // If we aren't doing a quiet run, print some info about the finished mix to
    // the console.
    if (!Quiet) {
        printf("Header is %d bytes, body is %d bytes, MIX contains %d files.\n", headersize, bodysize, FileIndex.Count());
    }
}

template<class FC, typename CRC>
int MixFileCreatorClass<FC, CRC>::Write_File(const char *filename, Pipe *pipe)
{
    FC filereader(filename);
    uint8_t buffer[1024];
    int data_read;
    int total = 0;

    filereader.Open(FM_READ);

    // We loop through the file reading chunks into our buffer and then writing
    // them to the destination pipe.
    while ((data_read = filereader.Read(buffer, 1024)) > 0) {
        total += pipe->Put(buffer, data_read);
    }

    filereader.Close();

    return total;
}
