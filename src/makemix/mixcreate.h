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

template<typename FC, typename CRC>
class MixFileCreatorClass
{
private:
    class FileDataNode : public Node<FileDataNode *>
    {
    public:
        FileDataNode() : Node<FileDataNode *>(), m_entry(), m_filePath(nullptr), m_inMix(false) {}
        ~FileDataNode();

    public:
        FileInfoStruct m_entry;
        char *m_filePath;
        bool m_inMix;
    };

public:
    MixFileCreatorClass(char const *filename, bool has_checksum = false, bool is_encrypted = false, bool quiet = true,
        bool force_flags = false);
    ~MixFileCreatorClass();

    char const *Get_Filename() const
    {
        char const *name = m_newMixHandle->File_Name();
        if (name == nullptr) {
            return s_tempFilename;
        }
        return name;
    }

    char const *Get_Temp_Filename() const { return s_tempFilename; };
    void Set_Filename(char const *filename) { m_newMixHandle->Set_Name(filename); }
    void Add_File(char const *filename);
    void Add_Files(char const *path);
    void Remove_File(char const *filename);
    void Write_Mix();

private:
    static char s_tempFilename[];

protected:
    List<FileDataNode *> m_fileList;
    IndexClass<int32_t, FileDataNode *> m_fileIndex;
    FC *m_newMixHandle;
    bool m_hasChecksum;
    bool m_isEncrypted;
    bool m_forceFlags;
    bool m_quiet;
    MIXHeaderStruct m_header;

private:
    int Write_File(char const *filename, Pipe *pipe);
};

template<class FC, typename CRC>
char MixFileCreatorClass<FC, CRC>::s_tempFilename[] = "makemix.mix";

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::FileDataNode::~FileDataNode()
{
    if (m_filePath) {
        free(m_filePath);
    }
    m_filePath = nullptr;
}

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::MixFileCreatorClass(
    char const *filename, bool has_checksum, bool is_encrypted, bool quiet, bool force_flags) :
    m_fileList(),
    m_fileIndex(),
    m_newMixHandle(nullptr),
    m_hasChecksum(has_checksum),
    m_isEncrypted(is_encrypted),
    m_forceFlags(force_flags),
    m_quiet(quiet),
    m_header()
{
    // Create a new file instance and store it.
    m_newMixHandle = filename != nullptr ? new FC(filename) : new FC(s_tempFilename);
}

template<class FC, typename CRC>
MixFileCreatorClass<FC, CRC>::~MixFileCreatorClass()
{
    if (m_newMixHandle) {
        delete m_newMixHandle;
    }

    m_newMixHandle = nullptr;
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Add_File(char const *filename)
{
    char path[PATH_MAX];
    path[PATH_MAX - 1] = '\0';

    FC file(filename);

    if (!file.Is_Available() || m_header.file_count == USHRT_MAX || (m_header.file_size + (uint64_t)file.Size()) > ULONG_MAX) {
        return;
    }

    strlcpy(path, filename, sizeof(path));
    FileDataNode *filedata = new FileDataNode;

    filedata->m_filePath = strdup(path);

#ifdef PLATFORM_WINDOWS
    char basename[_MAX_FNAME];
    basename[_MAX_FNAME - 1] = '\0';

    char ext[_MAX_EXT];
    ext[_MAX_EXT - 1] = '\0';

    _splitpath(path, nullptr, nullptr, basename, ext);
    snprintf(path, PATH_MAX, "%s%s", basename, ext);

    filedata->m_entry.m_CRC = Calculate_CRC<CRC>(strupr(path), (unsigned)strlen(path));
#else // !PLATFORM_WINDOWS
    char *basefname = basename(path); // basename can alter string so assign after duplicated
    filedata->m_entry.m_CRC = Calculate_CRC<CRC>(strupr(basefname), (unsigned)strlen(basefname));
#endif // PLATFORM_WINDOWS

    filedata->m_entry.m_Offset = m_header.file_size;
    filedata->m_entry.m_Size = file.Size();

    m_fileList.Add_Tail(filedata);
    m_fileIndex.Add_Index(filedata->m_entry.m_CRC, filedata);

    filedata->m_entry.m_CRC = htole32(filedata->m_entry.m_CRC);
    filedata->m_entry.m_Offset = htole32(filedata->m_entry.m_Offset);
    filedata->m_entry.m_Size = htole32(filedata->m_entry.m_Size);

    m_header.file_size += file.Size();
    ++m_header.file_count;
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

    FileDataNode *filedata = m_fileIndex.Fetch_Index(id);

    if (!filedata) {
        return;
    }

    m_header.file_size -= filedata->m_entry.m_Size;
    --m_header.file_count;
    filedata->Unlink();
    m_fileIndex.Remove_Index(id);

    delete filedata;
#endif
}

template<class FC, typename CRC>
void MixFileCreatorClass<FC, CRC>::Write_Mix()
{
    MIXHeaderStruct tmpheader;
    SHAEngine::SHADigest digest;

    PKPipe pkpipe(PIPE_ENCRYPT, g_CryptRandom);
    SHAPipe shpipe;
    FilePipe flpipe(m_newMixHandle);

    Pipe *pipe_to_use = &flpipe;

    int headersize = 0;
    int bodysize = 0;

    m_newMixHandle->Open(FM_WRITE);
    pkpipe.Put_To(&flpipe);
    shpipe.Put_To(&flpipe);

    // Write the flags and setup PKPipe if needed.
    if (m_isEncrypted || m_hasChecksum || m_forceFlags) {
        uint32_t flags = 0;

        if (m_isEncrypted) {
            pkpipe.Key(&g_PrivateKey);
            pipe_to_use = &pkpipe;
            flags |= IS_ENCRYPTED;
        }

        if (m_hasChecksum) {
            flags |= HAS_CHECKSUM;
        }

        flags = htole32(flags);

        // Only use FilePipe to put the flags.
        headersize += flpipe.Put(&flags, sizeof(flags));
    }

    // Write the header, swapping byte order if needed.
    tmpheader.file_count = htole16(m_header.file_count);
    tmpheader.file_size = htole32(m_header.file_size);

    headersize += pipe_to_use->Put(&tmpheader, sizeof(tmpheader));

    // This forces a sort of the index which is important to get the header written in the correct order.
    m_fileIndex.Is_Present(0);

    // Write the index, the index should have been byte swapped as it was
    // generated if it was needed.
    for (int i = 0; i < m_fileIndex.Count(); ++i) {
        headersize += pipe_to_use->Put(&m_fileIndex.Fetch_Data_By_Index(i)->m_entry, sizeof(FileInfoStruct));
    }

    headersize += pipe_to_use->Flush();

    // Change to either SHAPipe or plain FilePipe depending on if we need a
    // checksum.
    pipe_to_use = m_hasChecksum ? static_cast<Pipe *>(&shpipe) : static_cast<Pipe *>(&flpipe);

    // Loop through the file nodes and write the file data to the body of the
    // mix. These are written in the order they were added to the list, not the
    // sorted order that the index must be written in.
    for (FileDataNode *node = m_fileList.First(); node->Next() != nullptr; node = node->Next()) {
        if (!m_quiet) {
            printf("Writing file %s\n", node->m_filePath);
        }

        bodysize += Write_File(node->m_filePath, pipe_to_use);
    }

    // Write the checksum if needed, this will have been generated during the
    // writing of the mix body if it was required.
    if (m_hasChecksum) {
        shpipe.Result(&digest);
        flpipe.Put(&digest, sizeof(digest));
    }

    // If we aren't doing a quiet run, print some info about the finished mix to
    // the console.
    if (!m_quiet) {
        printf("Header is %d bytes, body is %d bytes, MIX contains %d files.\n", headersize, bodysize, m_fileIndex.Count());
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
